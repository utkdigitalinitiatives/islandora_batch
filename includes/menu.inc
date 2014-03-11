<?php

function islandora_batch_get_mappings(&$form_state) {
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
  $form_state['stash']['keys'] = array_flip($keys);

  return array($keys, $options);
}
function islandora_batch_map_value($value, $form_state) {
  return $form_state['stash']['keys'][$value];
}

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
  list($keys, $options) = islandora_batch_get_mappings($form_state);

  $current = db_select('islandora_batch_state', 's')
    ->fields('s', array('state'))
    ->condition('id', $id)
    ->execute()
    ->fetchField();
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
    ->fields(array('state' => islandora_batch_map_value($state, $form_state)))
    ->condition('id', $id)
    ->execute();
  drupal_set_message(t('Updated "@item" to "@state", in queue.', array(
    '@item' => $id,
    '@state' => $form['state']['#options'][$state],
  )));
}

function islandora_batch_set_set_items_state_page_callback($id) {
  return drupal_get_form('islandora_batch_set_set_items_state_form', $id);
}

function islandora_batch_set_set_items_state_form($form, &$form_state, $id) {
  form_load_include($form_state, 'inc', 'islandora_batch', 'includes/menu');
  list($keys, $options) = islandora_batch_get_mappings($form_state);

  $form['source_state'] = array(
    '#type' => 'select',
    '#title' => t('Change from'),
    '#description' => t('Items in the set with this state will be changed.'),
    '#empty_option' => t('-- Any --'),
    '#empty_value' => 'any',
    '#options' => $options,
  );
  $form['dest_state'] = array(
    '#type' => 'select',
    '#title' => t('Change to'),
    '#description' => t('Items will be set to this state.'),
    '#options' => $options,
  );

  return confirm_form($form, t('Are you sure you want to change the state of items in set "@set" in the queue?', array(
    '@set' => $id,
  )), "islandora_batch/reports/set");
}

function islandora_batch_set_set_items_state_form_submit(&$form, &$form_state) {
  list($id) = $form_state['build_info']['args'];
  $source_state = $form_state['values']['source_state'];
  $dest_state = $form_state['values']['dest_state'];
  $s = db_select('islandora_batch_queue', 'q')
    ->fields('q', array('id'))
    ->condition('sid', $id);
  $q = db_update('islandora_batch_state')
    ->fields(array('state' => islandora_batch_map_value($dest_state, $form_state)))
    ->condition('id', $s, 'IN');

  if ($source_state !== 'any') {
    $q->condition('state', islandora_batch_map_value($source_state, $form_state));
  }

  $updated = $q->execute();

  drupal_set_message(format_plural($updated, 'Updated 1 item in queue to "@state".', 'Updated @count items in queue to "@state".', array(
    '@state' => $form['dest_state']['#options'][$dest_state],
  )));
}

function islandora_batch_delete_set_page_callback($id) {
  return drupal_get_form('islandora_batch_delete_set_form', $id);
}

function islandora_batch_delete_set_form($form, &$form_state, $id) {
  form_load_include($form_state, 'inc', 'islandora_batch', 'includes/menu');
  return confirm_form($form, t('Are you sure you want to delete the items in set "@set" from the queue?', array(
    '@set' => $id,
  )), "islandora_batch/reports/set");
}

function islandora_batch_delete_set_form_submit(&$form, &$form_state) {
  list($id) = $form_state['build_info']['args'];
  $transaction = db_transaction();
  $s = db_select('islandora_batch_queue', 'q')
    ->fields('q', array('id'))
    ->condition('sid', $id);
  db_delete('islandora_batch_state')
    ->condition('id', $s, 'IN')
    ->execute();
  db_delete('islandora_batch_resources')
    ->condition('id', $s, 'IN')
    ->execute();
  $deleted = db_delete('islandora_batch_queue')
    ->condition('sid', $id)
    ->execute();
  db_delete('islandora_batch_set')
    ->condition('id', $id)
    ->execute();
  drupal_set_message(format_plural($deleted, 'Deleted 1 item from queue.', 'Deleted @count items from queue.'));
}
