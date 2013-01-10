<?php

define('ISLANDORA_BATCH_LOCK_NAME', 'islandora_batch_ingest');
define('ISLANDORA_BATCH_TIMER_NAME', 'islandora_batch_timer');

function islandora_batch_get_average($context) {
  if ($context['results']['count'] > 0) {
    return $context['results']['sum'] / $context['results']['count'];
  }
}

function islandora_batch_ingest_process(array $parameters, array &$context) {
  extract($parameters);

  if (!isset($timeout)) {
    $max_exec = intval(ini_get('max_execution_time'));
    if ($max_exec > 0) {
      // max_execution_time could be 0 if run from CLI (drush?)
      $timeout = $max_exec;
    }
    else {
      // Lock for an hour by default.
      $timeout = 3600.0;
    }
  }

  $lock_timeout = $timeout;
  $finished = FALSE;

  timer_start(ISLANDORA_BATCH_TIMER_NAME);

  // Wait up to a third of our processing time to try to get the ingest lock.
  if (!lock_wait(ISLANDORA_BATCH_LOCK_NAME, $timeout / 3) && lock_acquire(ISLANDORA_BATCH_LOCK_NAME, $lock_timeout)) {
    // Try ingesting objects that are waiting for children, if all the children are ready.

    // Select all children which are NOT fully ingested.
    $child_query = db_select('islandora_batch_queue', 'c')
      ->fields('c')
      ->condition('c.parent', 'p.id')
      ->condition('c.state', 3, '<>');

    // Select all items which do not have children which are not fully
    //   ingested, but are waiting for children.
    $part_query = db_select('islandora_batch_queue', 'p')
      ->fields('p')
      ->condition('p.state', 2)
      ->notExists($child_query);

    $finished = islandora_batch_process_results($part_query->execute());

    // If the last part finished completely... (due to timeout, if it didn't).
    if ($finished) {
      // ... Continue processing.

      // Select other objects which are ready to be processed.
      $other_query = db_select('islandora_batch_queue', 'q')
        ->fields('q')
        ->condition('state', 1);
      $finished = islandora_batch_process_results($other_query->execute());
    }
  }

  if (!$finished && isset($parameters['timeout']) && !islandora_batch_time_check(ISLANDORA_BATCH_TIMER_NAME, $timeout  / 3, $timeout)) {
    // If we're not completely finished but we're ingesting for a limited
    //   amount of time and we do not have enough time to try acquiring the
    //   lock again, inform the batch process so it stops.
    $context['finished'] = 1;
  }
  else {
    // Otherwise, make us get scheduled again.
    $context['finished'] = 0;
  }

  timer_stop(ISLANDORA_BATCH_TIMER_NAME);
}

/**
 * Check if we have enough time to perform "next_op", with a bit of padding.
 */
function islandora_batch_time_check($timer, $next_op, $timeout) {
  return (timer_read($timer) + $next_op) / 1000 < $timeout + 5;
}

/**
 * Process set of result from the islandora_batch_queue table.
 *
 * @param $results
 *   The query results.
 * @param array $context
 *   The array of batch context.
 *
 * @return boolean
 *   Whether or not we managed to finish this set.
 */
function islandora_batch_process_results($results, &$context) {
  $average_time = -1;

  while((
      $average_time < 0 ||
      islandora_batch_time_check(ISLANDORA_BATCH_TIMER_NAME, $average_time, $timeout)
    ) &&
    $object = $results->fetchAssoc()
  ) {

    $start = timer_read(ISLANDORA_BATCH_TIMER_NAME);

    // TODO: Process a single object.
    $ingest_object = unserialize($object['data']);
    $object['state'] = $ingest_object->batch_process();

    if ($object['state'] === 3) {
      // Push to backend.
      $ingested_object = islandora_add_object($ingest_object);
      $object['data'] = serialize($ingested_object);
    }

    // Update the info in the database.
    db_merge('islandora_batch_queue')
      ->key('id')
      ->fields($object)
      ->execute();

    $end = timer_read(ISLANDORA_BATCH_TIMER_NAME);
    $context['results']['count']++;
    $context['results']['sum'] += $end - $start;
    $average_time = islandora_batch_get_average($context);
  }

  // $object should be "FALSE" if we finished this set of object.
  return $object === FALSE;
}

/**
 * Batch 'finished' callback.
 */
function islandora_batch_finished($success, $results, $operations) {
  lock_release(ISLANDORA_BATCH_LOCK_NAME);
}

