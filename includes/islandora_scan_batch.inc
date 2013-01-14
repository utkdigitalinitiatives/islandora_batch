<?php

/**
 * A class which is capable of getting the listing of a directory or zip file.
 *
 * Used as a basis for the book batch. Really, just adds a couple methods to
 * help out.
 */
class IslandoraScanBatch extends IslandoraBatchPreprocessor {
  /**
   * Get a listing of "file-object"-like entries.
   *
   * @return array
   *   An associative array of stdClass objects representing files. Array keys
   *   are URIs, and the objects properties include:
   *   - uri: A string containing the URI of the resource (also used a the
   *     array key).
   *   - filename: The full filename.
   *   - name: The filename without its extension.
   */
  protected function scan() {
    $method = "scan_" . $this->get_target_type();
    return $this->$method();
  }

  /**
   * Get content models.
   */
  private function get_content_models() {
    return $this->parameters['content_models'];
  }

  /**
   * Get the target resource.
   */
  protected function get_target() {
    return $this->parameters['target'];
  }

  /**
   * Get the type of the target resource.
   *
   * Prefixed with "scan_" to determine which method gets called to generate
   * the list of resource.
   */
  protected function get_target_type() {
    return $this->parameters['type'];
  }

  /**
   * Scan the directory with file_scan_directory().
   */
  protected function scan_directory() {
    $files = file_scan_directory($this->get_target(), '/.*/');
    return $files;
  }

  /**
   * Generate output analogous to file_scan_directory().
   */
  protected function scan_zip() {
    $zip = new ZipArchive();

    $zip_file = drupal_realpath($this->get_target());
    if (($error = $zip->open($zip_file)) !== TRUE) {
      return FALSE;
    }

    $count = $zip->numFiles;

    $info = array();
    for ($i = 0; $i < $count; $i++) {
      $name = $zip->getNameIndex($i);
      $path_info = pathinfo($name);

      if (strpos($path_info['dirname'], '__MACOSX') === FALSE) {
        $file_entry = new stdClass();
        $file_entry->uri = "zip://$zip_file#$name";
        $file_entry->filename = $name;
        $file_entry->name = $path_info['filename'];
        $info[$file_entry->uri] = $file_entry;
      }
    }

    $zip->close();

    return $info;
  }

  /**
   * Group file entries logically, to pass off to the import object.
   *
   * Default implementation groups on files' basename.
   *
   * @param array $files
   *   An array, as returned by file_scan_directory().
   */
  protected function group_files($files) {
    $grouped = array();

    // TODO: Group according to basename, to and hand-off each group to object
    //  wrappers.
    foreach ($files as $uri => &$file_object) {
      $file_object->ext = pathinfo($file_object->filename, PATHINFO_EXTENSION);
      $grouped[$file_object->name][$file_object->ext] = $file_object;
    }

    return $grouped;
  }

  /**
   * Get the name of the class to instantiate for the batch operations.
   */
  protected static function get_object_class() {
    return "IslandoraScanBatchObject";
  }

  private function add_content_model_relationships(AbstractFedoraObject $object) {
    $object->models = $this->parameters['content_models'];
  }

  protected function add_relationships(AbstractFedoraObject $object) {
    $collection = $this->parameters['parent'];
    $predicate = $this->parameters['parent_relationship_pred'];
    $uri = $this->parameters['parent_relationship_uri'];

    $object->relationships->add($uri, $predicate, $collection);

    $this->add_content_model_relationships($object);
  }

  /**
   * Perform preprocessing of the scanned resources.
   */
  public function preprocess() {
    $files = $this->scan();

    if ($files === FALSE) {
      // Failed to scan.
      // TODO: Pop an error.
    }
    elseif (empty($files)) {
      // No contents.
      // TODO: Pop a warning.
    }

    $grouped = $this->group_files($files);
    $object_class = static::get_object_class();
    foreach ($grouped as $base_name => $object_info) {
      $ingest_object = new $object_class($this->connection, $base_name, $object_info);

      $this->add_relationships($ingest_object);

      $id = $this->add_to_database($ingest_object, $ingest_object->get_resources());

      // XXX: Might be better to have this actually handled as another "preprocessor",
      //   so arbitrary "tree" structures might be built?
      foreach ($this->get_children($id, $object_info) as $child) {
        $this->add_to_database($ingest_object, $child->get_resources(), $id);
      }
    }
  }

  /**
   * Get an array containing a number of ingest objects representing the children.
   *
   * Only (currently) used in a subclass.
   *
   * @param string $id
   *   A string containing the backend-id of the parent.
   * @param array $object_info
   *   The information about the parent, from a single "group".
   *
   * @return array
   *   An array of IslandoraBatchObjects.
   */
  protected function get_children($id, $object_info) {
    // TODO: Get an array containing a number of ingest objects representing
    //  the children, given the information about the parent.
    return array();
  }
}

class IslandoraScanBatchObject extends IslandoraBatchObject {
  protected $base_name, $object_info;

  public function __construct(IslandoraTuque $connection, $base_name, $object_info) {
    parent::__construct(NULL, $connection->repository);

    $this->base_name = $base_name;
    $this->object_info = $object_info;
    $this->resources = array();
  }

  protected static function get_mime($file_object) {
    static $mime_detect = NULL;
    if ($mime_detect === NULL) {
      $mime_detect = new MimeDetect();
    }

    return $mime_detect->getMimetype($file_object->filename);
  }

  public function batch_process() {
    // Use object_info to create some datastreams.
    $this->label = $this->get_title();
    $this->get_mods();
    $this->get_dc();

    if (!isset($this['OBJ'])) {
      $meta = array_map(NULL, array('xml', 'mrc'));
      $other = array_diff_key($this->object_info, $meta);

      if ($obj = reset($other)) {
        $obj_datastream = $this->constructDatastream('OBJ');
        // Figure out the mimetype.
        $obj_datastream->mimetype = static::get_mime($obj);
        $obj_datastream->label = 'OBJ Datastream';
        $obj_datastream->setContentFromFile($obj->uri, FALSE);

        $this->ingestDatastream($obj_datastream);
      }
    }

    // XXX: Should probably use constants...
    return 3;
  }

  /**
   * Get resources for the current object.
   */
  public function get_resources() {
    return $this->resources;
  }

  /**
   * Get the title for this object.
   *
   * Gets the label not empty; otherwise, grabs a title from the MODS.
   */
  protected function get_title() {
    if (!isset($this->label) || empty($this->label)) {
      // TODO: Acquire a title.
      $mods = $this->get_mods();
      if ($mods) {
        $mods_doc = new DOMDocument();
        $mods_doc->loadXML($mods);
        $mods_xpath = new DOMXPath($mods_doc);
        $mods_xpath->registerNamespace('m', 'http://www.loc.gov/mods/v3');

        $this->label = $mods_xpath->evaluate('string(//m:mods/m:titleInfo/m:title/text())');
      }
    }

    return $this->label;
  }

  /**
   * Run an XSLT, and return the results.
   *
   * @param array $info
   *   An associative array of parameters, containing:
   *   - input: The input XML in a string.
   *   - xsl: The path to an XSLT file.
   *
   * @return string
   *   The transformed XML, as a string.
   */
  protected static function run_xsl_transform($info) {
    $xsl = new DOMDocument();
    $xsl->load($info['xsl']);

    $input = new DOMDocument();
    $input->loadXML($info['input']);

    $processor = new XSLTProcessor();
    $processor->importStylesheet($xsl);

    // XXX: Suppressing warnings regarding unregistered prefixes.
    return $processor->transformToXML($input);
  }

  protected function get_mods() {
    if (!isset($this['MODS'])) {
      $dir = drupal_get_path('module', 'islandora_batch');
      // TODO: Create the MODS datastream on this object.
      $mods_datastream = $this->constructDatastream('MODS', 'X');
      $mods_datastream->mimetype = 'text/xml';
      $mods_datastream->label = 'MODS Record';

      // If we have an XML stream which is:
      if (isset($this->object_info['xml'])) {
        $xml = file_get_contents($this->object_info['xml']->uri);

         if ($xml) {
          $s_xml = new SimpleXMLElement($xml);
      //   MODS, set
          if ($s_xml->getName() == 'mods') {
            // XXX: Should probably create a resource entry.
            $mods_datastream->setContentFromFile($this->object_info['xml']->uri, FALSE);
          }
      //   MARCXML, transform to MODS and set
          elseif ($s_xml->getName() == 'record') {
            $mods_datastream->content = static::run_xsl_transform(array(
              'input' => $xml,
              'xsl' => $dir . '/transforms/MARC21slim2MODS3-4.xsl',
            ));
          }
      //   DC, transform to MODS and set
          elseif ($s_xml->getName() == 'dc') {
            $mods_datastream->content = static::run_xsl_transform(array(
              'input' => $xml,
              'xsl' => $dir . '/transforms/simpleDC2MODS.xsl',
            ));
          }
      //   Unrecognized format...
          else {
            // Do nothing?
          }
        }
      }

      // We use some Java code from LoC to transform binary MARC to MODS.
      if (!isset($mods_datastream->content) && isset($this->object_info['mrc'])) {
        // Generate some MODS from binary MARC.

        // Get the java executable.
        $java = variable_get('islandora_batch_java', 'java');
        $temp_file_marc = tempnam('/tmp', 'binmrc');
        $temp_file_mods = tempnam('/tmp', 'mods');

        // Copy the file, so we're sure it's accessible on the filesystem.
        copy($this->object_info['mrc']->uri, $temp_file_marc);
        $absolute = realpath($dir);

        // The command to run.
        $command = "$java -cp $absolute/transforms/marcxml/marcxml.jar:$absolute/transforms/marcxml/marc4j.jar $temp_file_marc $temp_file_mods";

        // Run the command.
        $proc = popen($command, 'r');
        pclose($proc);

        // Load the output into a DOMDocument.
        $doc = new DOMDocument();
        $doc->load($temp_file_mods);

        // Drop the temp.
        unlink($temp_file_marc);
        unlink($temp_file_mods);

        $mods_docs = $doc->getElementsByTagName('mods');
        if ($mods_docs->length > 0) {
          // Grab the first.
          $mods = new DOMDocument();
          $mods_node = $mods_docs->item(0);
          $mods_node =  $mods->importNode($mods_node, TRUE);
          $mods->appendChild($mods_node);

          // TODO: Transform dates to Solr format...  Or should this be left to
          //   a core hook?

          // And put the content into the datastream.
          $mods_datastream->content = $mods->saveXML();
        }
      }

      if (!isset($mods_datastream->content)) {
      // If we do not yet have any XML, create a dumb MODS with just the
      //   title, and set it...
        $title = $this->base_name;

        $mods_datastream->content = <<<EOXML
<mods xmlns:mods="http://www.loc.gov/mods/v3" xmlns="http://www.loc.gov/mods/v3">
  <titleInfo>
    <title>$title</title>
  </titleInfo>
</mods>
EOXML;
      }

      $this->ingestDatastream($mods_datastream);
    }

    return $this['MODS']->content;
  }

  protected function get_dc() {
    if (!isset($this['DC'])) {
      // TODO: Create the DC datastream on this object.
      $dc_datastream = $this->constructDatastream('DC', 'X');
      $dc_datastream->mimetype = 'text/xml';
      $dc_datastream->label = 'DC Record';

      if (isset($this->object_info['xml'])) {
        $xml = file_get_contents($this->object_info['xml']->uri);

        if ($xml) {
          $s_xml = new SimpleXMLElement($xml);
          if ($s_xml->getName() == 'dc') {
            // XXX: Should probably create a resource entry.
            $dc_datastream->setContentFromFile($this->object_info['xml']->uri, FALSE);
          }
        }
      }

      // Get the DC by transforming from MODS.
      if (!isset($dc_datastream->content)) {
        // XXX: Might want to make this use a file, instead of staging the DC
        //   in the database table (inside the object we serialize).
        $new_dc = static::run_xsl_transform(array(
          'xsl' => drupal_get_path('module', 'islandora_batch') . '/transforms/mods_to_dc.xsl',
          'input' => $this->get_mods(),
        ));
        if ($new_dc) {
          $dc_datastream->content = $new_dc;
        }
      }

      $this->ingestDatastream($dc_datastream);
    }

    return $this['DC']->content;
  }
}
