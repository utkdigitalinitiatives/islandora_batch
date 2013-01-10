<?php

class IslandoraBatchFailedToLockException extends Exception {}

abstract class IslandoraBatchPreprocessor {
  /**
   * @var array $parameters
   *   Should be populated with the parameters passed in.
   */
  protected array $parameters = array();

  /**
   * @var IslandoraTuque $connection
   *   An IslandoraTuque object from which we can obtain IDs.
   */
  protected IslandoraTuque $connection; 

  /**
   * Constructor must be able to recieve an associative array of parameters.
   *
   * @param array $parameters
   *   An associative array of parameters for the batch process. These will
   *   probably just be the result of a simple transformation from the
   *   command line, or something which could have been constructed from a
   *   form. (Some) base parameter(s):
   *   - timeout: The process should end after this length of time.
   */
  public function __construct(IslandoraTuque $connection, array $parameters) {
    $this->parameters = $parameters;
    $this->connection = $connection;

    $timeout = $this->parameters['timeout'];

    $lock = lock_acquire('islandora_batch_processing', $timeout);
    if (!$lock) {
      throw new IslandoraBatchFailedToLockException(
        t('Failed to acquire lock for batch ingest.')
      );
    }
  }

  /**
   * Actually cause the preprocessing to happen.
   *
   * In most cases, this should be fairly quick--not requiring multiple
   * iterations as a batch process.
   */
  abstract public function preprocess();

  /**
   * Get an identifier we can use for an object.
   *
   * @return string
   *   An id to use in the back-end (Fedora).
   */
  protected function get_id() {
    // Not specifying a namespace should result in one from the default
    //   declared in the fedora.fcfg ("changeme" by default).
    // TODO: Implement some form of caching, so we can get multiple at a time.
    return $connection->repository->api->m->getNextPid();
  }

  /**
   * Add the provided object to the database.
   *
   * Should be called from inside the "preprocess" method.
   *
   * @param IslandoraBatchObject $object
   *   The object to be later processed and added to Fedora.
   * @param array $resources
   *   An associative array of arrays, mapping types of resources to an array
   *   of relevant resources for the given object.
   *
   * @return string
   *   The PID of the object (one will be allocated if the object does not
   *   already have one; otherwise, that on the object will be returned).
   */
  protected function add_to_database(AbstractFedoraObject $object,
    array $resources = array(), $parent = NULL) {

    if (!isset($object->id)) {
      $object->id = $this->get_id();
    }

    db_insert('islandora_batch_queue')
      ->fields(array('id', 'parent', 'data'))
      // Could queue up multiple in a static cache, to dump all together?
      ->values(array($object->id, $parent, serialize($object)))
      ->execute();
    db_insert('islandora_batch_state')
      ->fields(array('id', 'state'))
      ->values(array($object->id, $object->initial_batch_state()))
      ->execute();

    foreach ($resources as $type => $typed_resources) {
      foreach ($typed_resources as $resource) {
        db_insert('islandora_batch_resources')
          ->fields(array('id', 'type', 'resource'))
          ->values(array($object->id, $type, $resource))
          ->execute();
      }
    }

    return $object->id;
  }

  /**
   * Add the relationship to a top-level object.
   */
  protected function add_membership_relationship(AbstractFedoraObject $object) {
    $collection = isset($this->parameters['collection'])?
      $this->parameters['collection']:
      variable_get('islandora_repository_pid', 'islandora:root');
    $predicate = 'isMemberOfCollection';
    $uri = FEDORA_RELS_EXT_URI;

    $object->relationships->add($uri, $predicate, $collection);
  }
}

