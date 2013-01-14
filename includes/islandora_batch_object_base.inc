<?php

/**
 * Batch interface.
 *
 * Implementing classes should subclass some version of FedoraObject, so 
 */
abstract class IslandoraBatchObject extends IslandoraNewFedoraObject {
  /**
   * @return int
   *   Used to determine the initial state of the object in the queue.
   *   - 0: Not ready to be processed.
   *   - 1: Ready to be processed.
   *   - 2: Partially processed (waiting for children to be processed).
   *   - 3: Fully processed... Push to the back-end.
   */
  public function initial_batch_state() {
    return 1;
  }

  /**
   * Perform arbitrary processing before ingest.
   *
   * @return int
   *   Return the state after processing. If it is "fully processed", this
   *   object should be pushed to the back-end.
   *   If the state is "partially processed", we should be careful that the
   *   work does not get performed again next time around.
   */
  abstract public function batch_process();

  /**
   * Get the resources to which we should link in the database.
   *
   * Should be in the format used by IslandoraBatchPreprocessor::add_to_database().
   */
  abstract public function get_resources();
}

