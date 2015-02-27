<?php

/**
 * @file
 * Database Helper Functions.
 */

/**
 * Log object details into drupal database.
 *
 * @param AbstractObject $object
 *   The object being processed for ingestion.
 * @param string $content_model
 *   The content model of the object being ingested.
 * @param string $state
 *   An integer identifying a ingest processing status; 0 for failed, and 1
 *   for successfully ingested.
 */
function islandora_batch_report_log_object($object, $content_model, $state) {
  db_insert('islandora_batch_report_objects_processed')->fields(array(
    'pid' => $object->id,
    'contentmodel' => $content_model,
    'label' => $object->label,
    'successful' => (int) $state,
    'timestamp' => REQUEST_TIME,
  ))->execute();
}

/**
 * Log set details into drupal database.
 *
 * @param int $set_id
 *   An integer identifying a batch set.
 * @param int $sucessful
 *   An integer identifying a batch set processing status; 0 for failed, and 1
 *   for successfully ingested.
 */
function islandora_batch_report_log_set($set_id, $sucessful) {
  db_insert('islandora_batch_report_sets_processed')->fields(array(
    'sid' => $set_id,
    'successful' => $sucessful,
    'timestamp' => REQUEST_TIME,
  ))->execute();
}

/**
 * Check if a set has already been added to the database with the same status.
 *
 * @param int $set_id
 *   An integer identifying a batch set.
 * @param int $sucessful
 *   An integer identifying a batch set processing status; 0 for failed, and 1
 *   for successfully ingested.
 *
 * @return results
 *   The results of the query.
 */
function islandora_batch_report_check_if_set_was_added($set_id, $sucessful) {
  $start_of_day = strtotime("midnight", REQUEST_TIME);
  $end_of_day   = strtotime("tomorrow", $start_of_day) - 1;

  $results = db_select('islandora_batch_report_sets_processed', 'q')
    ->fields('q', array('id'))
    ->condition('sid', $set_id)
    ->condition('successful', $sucessful)
    ->condition('timestamp', array($start_of_day, $end_of_day), 'BETWEEN')
    ->execute()
    ->fetchField();

  return $results;
}

/**
 * Get the count of sets based on the set processing status for a specified day.
 *
 * @param int $sucessful
 *   An integer identifying a batch set processing status; 0 for failed, and 1
 *   for successfully ingested.
 * @param int $timestamp
 *   A timestamp for filtering processed data.
 *
 * @return int
 *   The count of sets that match the requested filters.
 */
function islandora_batch_report_get_daily_set_count($sucessful, $timestamp) {
  $start_of_day = strtotime("midnight", $timestamp);
  $end_of_day   = strtotime("tomorrow", $start_of_day) - 1;

  $count = db_select('islandora_batch_report_sets_processed', 'q')
    ->fields('q', array())
    ->condition('successful', $sucessful)
    ->condition('timestamp', array($start_of_day, $end_of_day), 'BETWEEN')
    ->countQuery()
    ->execute()
    ->fetchField();

  return $count;
}

/**
 * Get the count of objects based on the processing status for a specified day.
 *
 * @param int $sucessful
 *   An integer identifying the object processing status; 0 for failed, and 1
 *   for successfully ingested.
 * @param int $timestamp
 *   A timestamp for filtering processed data.
 *
 * @return int
 *   The count of objects that match the requested filters.
 */
function islandora_batch_report_get_daily_object_count($sucessful, $timestamp) {
  $start_of_day = strtotime("midnight", $timestamp);
  $end_of_day   = strtotime("tomorrow", $start_of_day) - 1;

  $count = db_select('islandora_batch_report_objects_processed', 'q')
    ->fields('q', array())
    ->condition('successful', $sucessful)
    ->condition('timestamp', array($start_of_day, $end_of_day), 'BETWEEN')
    ->countQuery()
    ->execute()
    ->fetchField();

  return $count;
}

/**
 * Get the count of objects for each type of content model.
 *
 * @param int $sucessful
 *   An integer identifying the object processing status; 0 for failed, and 1
 *   for successfully ingested.
 * @param int $timestamp
 *   A timestamp for filtering processed data.
 *
 * @return string
 *   A sting break down of counts for each content model.
 */
function islandora_batch_report_get_daily_object_count_by_content_model($sucessful, $timestamp) {
  $content_model_details = "";
  $start_of_day = strtotime('midnight', $timestamp);
  $end_of_day   = strtotime('tomorrow', $start_of_day) - 1;
  // Get each unique content model and count the occurances.
  $query = db_select('islandora_batch_report_objects_processed', 's')
    ->distinct()
    ->fields('s', array('contentmodel'))
    ->execute();
  while ($result = $query->fetchAssoc()) {
    $count = db_select('islandora_batch_report_objects_processed', 'q')
      ->fields('q', array(''))
      ->condition('successful', $sucessful)
      ->condition('contentmodel', $result['contentmodel'])
      ->condition('timestamp', array($start_of_day, $end_of_day), 'BETWEEN')
      ->countQuery()
      ->execute()
      ->fetchField();
    $object = islandora_object_load($result['contentmodel']);
    if ($object) {
      $content_model_details .= $count . " {$object->label}\n";
    }
  }
  return $content_model_details;
}
