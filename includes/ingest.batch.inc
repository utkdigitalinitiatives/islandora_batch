<?php

define('ISLANDORA_BATCH_LOCK_NAME', 'islandora_batch_ingest');
define('ISLANDORA_BATCH_TIMER_NAME', 'islandora_batch_timer');

function islandora_batch_get_average($context) {
  if ($context['results']['count'] > 0) {
    return $context['results']['sum'] / $context['results']['count'];
  }
}

function islandora_batch_ingest_process(array $parameters, &$context) {
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
  if (lock_acquire(ISLANDORA_BATCH_LOCK_NAME, $lock_timeout) || (!lock_wait(ISLANDORA_BATCH_LOCK_NAME, $timeout / 3) && lock_acquire(ISLANDORA_BATCH_LOCK_NAME, $lock_timeout))) {
    // Try ingesting objects that are waiting for children, if all the children are ready.

    // Select all children which are NOT fully ingested.
    $finished = islandora_batch_process_results(islandora_batch_get_parents(), $timeout, $context);

    // If the last part finished completely... (due to timeout, if it didn't).
    if ($finished) {
      // ... Continue processing.
      $finished = islandora_batch_process_results(islandora_batch_get_ready(), $timeout, $context);
    }

    if ($finished) {
      // Attempt to process any parents which might now be ready to go, if we have any time left.
      $finished = islandora_batch_process_results(islandora_batch_get_parents(), $timeout, $context);
    }
  }

  if (!$finished && isset($parameters['timeout']) && !islandora_batch_time_check(ISLANDORA_BATCH_TIMER_NAME, $timeout  / 3, $timeout)) {
    // If we're not completely finished but we're ingesting for a limited
    //   amount of time and we do not have enough time to try acquiring the
    //   lock again, inform the batch process so it stops.
    $context['finished'] = 1;
  }
  elseif ($finished) {
    // We finished the queue of objects.
    $context['finished'] = 1;
  }
  else {
    // Otherwise, make us get scheduled again.
    $context['finished'] = 0;
  }

  timer_stop(ISLANDORA_BATCH_TIMER_NAME);
}

function islandora_batch_get_parents() {
  // Select all children which are NOT fully ingested.
  $child_query = db_select('islandora_batch_queue', 'c')
    ->fields('c')
    ->where('c.parent = p.id')
    ->isNotNull('c.parent');
  $c_alias = $child_query->join('islandora_batch_state', 'cs',
    'c.id = cs.id');
  $child_query->condition('cs.state', ISLANDORA_BATCH_STATE__DONE, '<>');

  // Select all items which do not have children which are not fully
  //   ingested, but are waiting for children.
  $parent_query = db_select('islandora_batch_queue', 'p')
    ->fields('p')
    ->fields('ps', array('state'));
  $p_alias = $parent_query->join('islandora_batch_state', 'ps',
    'p.id = ps.id');
  $parent_query->condition('state', ISLANDORA_BATCH_STATE__PENDING_CHILDREN);

  $parent_query->notExists($child_query);

  return $parent_query->execute();
}

function islandora_batch_get_ready() {
  // Select other objects which are ready to be processed.
  $other_query = db_select('islandora_batch_queue', 'q')
    ->fields('q')
    ->fields('s', array('state'));
  $other_alias = $other_query->join('islandora_batch_state', 's',
    'q.id = s.id AND s.state = :s_state', array(':s_state' => ISLANDORA_BATCH_STATE__READY));

  return $other_query->execute();
}

/**
 * Check if we have enough time to perform "next_op", with a bit of padding.
 */
function islandora_batch_time_check($timer, $next_op, $timeout) {
  return (timer_read($timer) + $next_op) / 1000 < $timeout + 5;
}

/**
 * A test implementation, to dump contents...
 */
function test_islandora_batch_process_results($results, $timeout, &$context) {
  dlm($results->getQueryString(), 'Query');
  foreach ($results as $result) {
    dlm($result, $result->id);
  }
  return TRUE;
}
//*/

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
function islandora_batch_process_results($results, $timeout, &$context) {
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

    if ($object['state'] !== ISLANDORA_BATCH_STATE__DONE) {
      $object['state'] = $ingest_object->batch_process();
    }

    if ($object['state'] === ISLANDORA_BATCH_STATE__DONE) {
      // Push to backend.
      $ingested_object = islandora_add_object($ingest_object);
      if ($ingested_object) {
        $object['data'] = serialize($ingested_object);
      }
      else {
        //  Failed to ingest...  Write back to the database, and say it's ready.
        $object['data'] = serialize($ingest_object);
        $object['state'] = ISLANDORA_BATCH_STATE__READY;
      }
    }

    // Update the info in the database.
    db_merge('islandora_batch_queue')
      ->key(array('id' => $object['id']))
      ->fields(array(
        'data' => $object['data'],
      ))
      ->execute();
    db_merge('islandora_batch_state')
      ->key(array('id' => $object['id']))
      ->fields(array(
        'state' => $object['state'],
      ))
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

