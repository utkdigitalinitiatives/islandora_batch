<?php

function islandora_batch_delete_item_page_callback($id) {
  return drupal_get_form('islandora_batch_delete_item_form', $id);
}

function islandora_batch_delete_item_form($form, &$form_state, $id) {
  form_load_include($form_state, 'inc', 'islandora_batch', 'includes/menu');
  return confirm_form($form, t('Are you sure you want to delete "@item" from the ingest queue?', array(
    '@item' => $id,
  )), 'islandora_batch/reports/queue');
}

function islandora_batch_delete_item_form_submit(&$form, &$form_state) {
  list($id) = $form_state['build_info']['args'];
  $transaction = db_transaction();
  db_delete('islandora_batch_state')
    ->condition('id', $id)
    ->execute();
  db_delete('islandora_batch_resources')
    ->condition('id', $id)
    ->execute();
  db_delete('islandora_batch_queue')
    ->condition('id', $id)
    ->execute();
  drupal_set_message(t('Deleted "@item" from queue.', array(
    '@item' => $id,
  )));
}

function islandora_batch_set_item_state_page_callback($id) {
  return drupal_get_form('islandora_batch_set_item_state_form', $id);
}

function islandora_batch_set_item_state_form($form, &$form_state, $id) {
  $keys = array(
    ISLANDORA_BATCH_STATE__ERROR => 'error',
    ISLANDORA_BATCH_STATE__NOT_READY => 'not-ready',
    ISLANDORA_BATCH_STATE__READY => 'ready',
    ISLANDORA_BATCH_STATE__PENDING_CHILDREN => 'pending-children',
    ISLANDORA_BATCH_STATE__DONE => 'done',
  );
  $strings = array(
    ISLANDORA_BATCH_STATE__ERROR => t('Error'),
    ISLANDORA_BATCH_STATE__NOT_READY => t('Not ready to ingest'),
    ISLANDORA_BATCH_STATE__READY => t('Ready to ingest'),
    ISLANDORA_BATCH_STATE__PENDING_CHILDREN => t('Not ready to ingest; children pending'),
    ISLANDORA_BATCH_STATE__DONE => t('Ingested'),
  );
  $options = array();
  foreach ($keys as $key => $value) {
    $options[$value] = $strings[$key];
  }

  $current = db_select('islandora_batch_state', 's')
    ->fields('s', array('state'))
    ->condition('id', $id)
    ->execute()
    ->fetchField();
  $form_state['stash']['keys'] = array_flip($keys);
  $form['state'] = array(
    '#type' => 'select',
    '#title' => t('State'),
    '#description' => t('The state of the given object in the batch queue.'),
    '#default_value' => $keys[(int) $current],
    '#options' => $options,
  );
  return confirm_form($form, t('Are you sure you want to change the state of "@item" in the queue?', array(
    '@item' => $id,
  )), 'islandora_batch/reports/queue');
}

function islandora_batch_set_item_state_form_submit(&$form, &$form_state) {
  list($id) = $form_state['build_info']['args'];
  $state = $form_state['values']['state'];
  db_update('islandora_batch_state')
    ->fields(array('state' => $form_state['stash']['keys'][$state]))
    ->condition('id', $id)
    ->execute();
  drupal_set_message(t('Updated "@item" to "@state", in queue.', array(
    '@item' => $id,
    '@state' => $form['state']['#options'][$state],
  )));
}
