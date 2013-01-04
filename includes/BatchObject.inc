<?php

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
   *   Return the state after processing.
   */
  abstract public function batch_process();
}

