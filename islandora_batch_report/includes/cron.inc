<?php

/**
 * @file
 * Cron functions.
 */

/**
 * Email daily report to the list of email address set in the admin form.
 */
function islandora_batch_report_generate_daily_email_report() {
  module_load_include('inc', 'islandora_batch_report', 'includes/db');
  module_load_include('inc', 'islandora_batch', 'includes/db');
  $timestamp = time();

  // XXX: An issue with rules module where it will not output 0 value because
  // the logic checks using empty() on the value it means: 0, "0", 0.00 will not
  // output. A patch was put in to fix the issue by switching the !empty to
  // isset https://www.drupal.org/node/2087109 if this gets merged in there is
  // no need for shorthand if statments that are setting 0 to " 0".
  $successful_object_total = islandora_batch_report_get_daily_object_count(1, $timestamp);
  $failed_object_total = islandora_batch_report_get_daily_object_count(0, $timestamp);
  $successful_set_total = islandora_batch_report_get_daily_set_count(1, $timestamp);
  $failed_set_total = islandora_batch_report_get_daily_set_count(0, $timestamp);
  $ready_to_process_sets = islandora_batch_get_count_of_queued_sets_ready_to_process();
  $ready_to_process_objects = islandora_batch_get_count_of_queued_objects_ready_to_process();

  rules_invoke_event('islandora_batch_report_send_report',
    $timestamp,
    ($successful_object_total == 0 ? " 0" : $successful_object_total),
    ($failed_object_total == 0 ? " 0" : $failed_object_total),
    ($successful_set_total == 0 ? " 0" : $successful_set_total),
    ($failed_set_total == 0 ? " 0" : $failed_set_total),
    islandora_batch_report_get_daily_object_count_by_content_model(1, $timestamp),
    islandora_batch_report_get_daily_object_count_by_content_model(0, $timestamp),
    ($ready_to_process_sets == 0 ? " 0" : $ready_to_process_sets),
    ($ready_to_process_objects == 0 ? " 0" : $ready_to_process_objects)
  );
}
