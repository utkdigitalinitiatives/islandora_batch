<?php
/**
 * @file
 * Views hook implementations.
 */

/**
 * Implements hook_views_data().
 */
function islandora_batch_views_data() {
  $data = array();

  $data['islandora_batch_queue'] = array(
    'table' => array(
      'group' => t('Islandora Batch'),
      'base' => array(
        'field' => 'id',
        'title' => t('Islandora Batch Queue'),
        'help' => t('The queue of objects to batch ingest.'),
      ),
      'join' => array(
        'islandora_batch_queue' => array(
          'left_field' => 'id',
          'field' => 'parent',
        ),
        'islandora_batch_set' => array(
          'left_field' => 'id',
          'field' => 'sid',
        ),
      ),
    ),
  );

  $data['islandora_batch_set'] = array(
    'table' => array(
      'group' => t('Islandora Batch'),
      'base' => array(
        'field' => 'id',
        'title' => t('Islandora Batch Set'),
        'help' => t('A set of items, to groups items from the queue.'),
      ),
      'join' => array(
        'users' => array(
          'left_field' => 'uid',
          'field' => 'uid',
        ),
      ),
    ),
  );

  $data['islandora_batch_state'] = array(
    'table' => array(
      'group' => t('Islandora Batch'),
      'join' => array(
        'islandora_batch_queue' => array(
          'left_field' => 'id',
          'field' => 'id',
        ),
      ),
    ),
  );

  return $data;
}
