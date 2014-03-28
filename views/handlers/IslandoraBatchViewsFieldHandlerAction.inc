<?php

/**
 * @file
 * 
 */

abstract class IslandoraBatchViewsFieldHandlerActions extends views_handler_field {
  protected function optionDefinition() {
    return parent::option_definition();
  }

  protected function optionsForm(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['redirect_base_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Redirect base path'),
      '#description' => t('The path to which users will be directed after performing a given action.'),
      '#default_value' => $this->redirectBasePath(),
      '#weight' => -101,
    );
  }

  protected function redirectBasePath() {
    return isset($this->options['redirect_base_path']) ?
      $this->options['redirect_base_path'] :
      'no/path/set';
  }

  protected function renderValues($values) {
    $views_module_path = drupal_get_path('module', 'views');

    $value = array(
      '#attached' => array(
        'css' => array(
          "$views_module_path/css/views-admin.ctools.css",
          "$views_module_path/css/views-admin.seven.css",
        ),
      ),
      '#prefix' => '<span class="ctools-dropbutton-wrapper">',
      '#suffix' => '</span>',
      '#theme' => 'links__ctools_dropbutton',
      '#links' => $this->getLinks($values),
    );
    return drupal_render($value);
  }

  /**
   * Get links to render as actions.
   *
   * @param object $values
   *   The other values available in the given row.
   *
   * @return array
   *   An array of links, as accepted in the 'links' parameter of
   *   theme_links(). Each should contain at least:
   *   - title: The link title.
   *   - href: The URL to which to link.
   */
  abstract protected function getLinks($values);

  // @codeStandardsIgnoreStart
  // XXX: views uses underscores in its function names... Let's map 'em, so the
  // code doing the lifting can actually be run through coder.
  function option_definition() {
    return $this->optionDefinition();
  }

  function options_form(&$form, &$form_state) {
    $this->optionsForm($form, $form_state);
  }

  function allow_advanced_render() {
    return FALSE;
  }

  function is_value_empty($value, $empty_zero, $no_skip_empty = TRUE) {
    // Since we don't really have a value (just some markup we always want to
    // render), let's short-circuit this.
    return FALSE;
  }

  function render($values) {
    return $this->renderValues($values);
  }

  function query() {
    // No-op.
  }
  // @codeStandardIgnoreEnd
}

class IslandoraBatchViewsFieldHandlerQueueActions extends IslandoraBatchViewsFieldHandlerActions {
  protected function optionDefinition() {
    $options = parent::optionDefinition();

    $options['redirect_base_path'] = array(
      'default' => 'islandora_batch/item',
      'translatable' => FALSE,
    );

    return $options;
  }

  protected function getLinks($values) {
    return array(
      array(
        'title' => t('Set item state'),
        'href' => format_string('!base/!item_id/set_state', array(
          '!base' => $this->redirectBasePath(),
          '!item_id' => $this->getItemId($values),
        )),
        'query' => drupal_get_destination(),
      ),
      array(
        'title' => t('Delete item'),
        'href' => format_string('!base/!item_id/delete', array(
          '!base' => $this->redirectBasePath(),
          '!item_id' => $this->getItemId($values),
        )),
        'query' => drupal_get_destination(),
      ),
    );
  }

  protected function getItemId($values) {
    return $values->id;
  }
}

class IslandoraBatchViewsFieldHandlerSetActions extends IslandoraBatchViewsFieldHandlerActions {
  protected function optionDefinition() {
    $options = parent::optionDefinition();

    $options['redirect_base_path'] = array(
      'default' => 'islandora_batch/set',
      'translatable' => FALSE,
    );
    $options['queue_path'] = array(
      'default' => 'islandora_batch/reports/queue',
      'translatable' => FALSE,
    );

    return $options;
  }

  protected function optionsForm(&$form, &$form_state) {
    parent::optionsForm($form, $form_state);

    $form['queue_path'] = array(
      '#type' => 'textfield',
      '#title' => t('The path for the queue interface'),
      '#description' => t('Needed to be able to generate a link to the items in the given set.'),
      '#default_value' => $this->getQueuePath(),
      '#weight' => -100,
    );
  }

  protected function getQueuePath() {
    return isset($this->options['queue_path']) ?
      $this->options['queue_path'] :
      'no/path/set';
  }

  protected function getLinks($values) {
    return array(
      array(
        'title' => t('View items in set'),
        'href' => format_string('!queue/!set_id', array(
          '!queue' => $this->getQueuePath(),
          '!set_id' => $this->getSetId($values),
        )),
      ),
      array(
        'title' => t('Set item states'),
        'href' => format_string('!base/!set_id/set_state', array(
          '!base' => $this->redirectBasePath(),
          '!set_id' => $this->getSetId($values),
        )),
        'query' => drupal_get_destination(),
      ),
      array(
        'title' => t('Delete set'),
        'href' => format_string('!base/!set_id/delete', array(
          '!base' => $this->redirectBasePath(),
          '!set_id' => $this->getSetId($values),
        )),
        'query' => drupal_get_destination(),
      ),
      array(
        'title' => t('Restart batch'),
        'href' => format_string('!base/!set_id/restart_batch', array(
          '!base' => $this->redirectBasePath(),
          '!set_id' => $this->getSetId($values),
        )),
        'query' => drupal_get_destination(),
      ),
    );
  }

  protected function getSetId($values) {
    return $values->id;
  }
}
