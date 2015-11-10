<?php

/**
 * @file
 * Contains the callback functions for citation MODS manipulations.
 */

/**
 * Grab the correct MODS edit form for citation objects and load it.
 *
 * @param AbstractObject $object
 *   The object to edit.
 *
 * @return array
 *   Returns the drupal form to load.
 */
function lib4ridora_citation_select_mods_edit_form(AbstractObject $object) {
  module_load_include('inc', 'xml_form_builder', 'includes/associations');
  module_load_include('inc', 'lib4ridora', 'includes/utilities');
  $associations = xml_form_builder_get_associations();
  $form_associations = lib4ridora_get_custom_citation_form_associations();
  $load_association = array();
  // Read Genre from MODS to determine what form to use for editing.
  $mods = new SimpleXMLElement($object['MODS']->content);
  $mods->registerXPathNamespace('mods', 'http://www.loc.gov/mods/v3');
  // Product Type.
  $type = $mods->xpath('/mods:mods/mods:genre');
  if (count($type) > 0) {
    $type = (string) reset($type);
    if (array_key_exists($type, $form_associations)) {
      $form_name = $form_associations[$type];
    }
  }
  if ($form_name) {
    foreach ($associations as $key => $association) {
      if ($association['form_name'] == $form_name) {
        // Update the content_model to be ir:citationCModel so that regardless
        // of the form used it will only have the ir:citationCModel.
        $association['content_model'] = "ir:citationCModel";
        module_load_include('inc', 'xml_form_builder', 'includes/datastream.form');
        return drupal_get_form('xml_form_builder_datastream_form_metadata_form', $object, $association);
      }
    }
  }

  // Fallback to load the ingest form if there is no current association so that
  // an association can be set.
  module_load_include('inc', 'lib4ridora', 'includes/ingest-selector.form');
  return drupal_get_form('lib4ridora_ingest_selector_form', $object);

}

/**
 * Used as a fallback to set the association and load the metadata form.
 *
 * @param AbstractObject $object
 *   The object to edit.
 *
 * @return array
 *   Returns the drupal form to load.
 */
function lib4ridora_citation_unknown_association_edit_form(AbstractObject $object) {
  session_start();
  if (isset($_SESSION['association'])) {
    $association = $_SESSION['association'];
    unset($_SESSION['association']);
    module_load_include('inc', 'xml_form_builder', 'includes/datastream.form');
    return drupal_get_form('xml_form_builder_datastream_form_metadata_form', $object, $association);
  }
  else {
    drupal_goto("islandora/object/{$object->id}/MODS/citation");
  }
}
