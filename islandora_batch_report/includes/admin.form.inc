<?php

/**
 * @file
 * The admin form.
 */

/**
 * Admin form for the batch.
 */
function islandora_batch_report_admin_form($form, $form_state) {
  $form['islandora_batch_report_email_list'] = array(
    '#type' => 'textarea',
    '#title' => t('Email Addresses'),
    '#description' => t('A comma separated list of email addresses to send daily batch ingest reports.'),
    '#default_value' => variable_get('islandora_batch_report_email_list', ''),
  );

  return system_settings_form($form);
}

/**
 * Admin form validate function.
 */
function islandora_batch_report_admin_form_validate($form, &$form_state) {
  if ($form_state['values']['islandora_batch_report_email_list']) {
    $email_addresses = explode(',', $form_state['values']['islandora_batch_report_email_list']);
    foreach ($email_addresses as $address) {
      $address = trim($address);
      if (!valid_email_address($address)) {
        form_set_error('islandora_batch_report_email_list', t('The e-mail address %address is not valid.', array('%address' => $address)));
      }
    }
  }
  else {
    form_set_error('islandora_batch_report_email_list', t('No e-mail address provided.'));
  }
}
