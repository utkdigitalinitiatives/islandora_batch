<?php

/**
 * @file
 * Cron functions.
 */

/**
 * Email daily report to the list of email address set in the admin form.
 */
function islandora_batch_report_daily_email_report() {
  // Get email address to send report.
  $islandora_batch_report_email_list = variable_get('islandora_batch_report_email_list', '');
  if ($islandora_batch_report_email_list) {
    $email_addresses = explode(',', $islandora_batch_report_email_list);
    foreach ($email_addresses as $address) {
      $address = trim($address);
      drupal_mail(
        'islandora_batch_report',
        'daily_batch_ingest_report',
        $address,
        language_default()
      );
    }
  }
}
