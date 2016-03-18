<?php

/**
 * @file
 * The admin form.
 */

/**
 * Admin form for the batch.
 */
function islandora_batch_admin_form($form, $form_state) {
  $form['islandora_batch_java'] = array(
    '#type' => 'textfield',
    '#title' => t('Java Executable'),
    '#description' => t('The Java executable to use to run MARC to MODS transformation.'),
    '#default_value' => variable_get('islandora_batch_java', 'java'),
  );
  $form['islandora_batch_link_when_finished'] = array(
    '#type' => 'checkbox',
    '#access' => module_exists('views'),
    '#title' => t('Link to queue when batch finishes?'),
    '#description' => t('When a batch finishes, we will attempt to render a link to review the queue for the given set. This is dependent on the "views" module being enabled.'),
    '#default_value' => variable_get('islandora_batch_link_when_finished', TRUE),
  );
  $form['islandora_batch_auto_remove_batch_set'] = array(
    '#type' => 'checkbox',
    '#title' => t('Auto-remove batch set when batch finishes?'),
    '#description' => t('When a batch finishes, it will be removed from storage.'),
  );

  return system_settings_form($form);
}
