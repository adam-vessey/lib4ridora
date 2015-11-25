<?php

/**
 * @file
 * XML form selector step form.
 */

/**
 * Form with ingest options to override the xml form association.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param bool $editing
 *   If the form is being called from editing.
 *
 * @return array
 *   The Drupal form state.
 */
function lib4ridora_ingest_selector_form(array $form, array &$form_state, $editing = FALSE) {
  $default = array(
    'ingest_selector' => 'Journal Article',
    'journal_import_method' => 'manual',
    'doi' => "",
  );
  if (!$editing) {
    // Load the defaults from the form step storage if returning to the form.
    $association_step_storage = &islandora_ingest_form_get_step_storage($form_state, 'xml_form_builder_association_step');
    if (isset($association_step_storage['journal_import_method'])) {
      $default['journal_import_method'] = $association_step_storage['journal_import_method'];
    }
    if (isset($association_step_storage['doi'])) {
      $default['doi'] = $association_step_storage['doi'];
    }
    if (isset($association_step_storage['ingest_selector'])) {
      $default['ingest_selector'] = $association_step_storage['ingest_selector'];
    }
  }
  $form['ingest_selector'] = array(
    '#title' => t('Select a Publication Type'),
    '#type' => 'radios',
    '#options' => array(
      "Journal Article" => t("Journal Article"),
      "Book Chapter" => t("Book Chapter"),
      "Book" => t("Book"),
      "Newspaper or Magazine Article" => t("Newspaper or Magazine Article"),
      "Conference Item" => t("Conference Item"),
      "Thesis" => t("Thesis"),
      "Patent" => t("Patent"),
      "Report" => t("Report"),
    ),
    "#default_value" => $default['ingest_selector'],
  );

  // Add descriptions to the radio buttons.
  $form['ingest_selector']['Journal Article'] = array(
    '#description' => t('An article in a scientific journal.  Not necessarily peer-reviewed. May be an electronic-only medium such as online journal.'),
  );
  $form['ingest_selector']['Book Chapter'] = array(
    '#description' => t('A chapter or contribution in a book.'),
  );
  $form['ingest_selector']['Book'] = array(
    '#description' => t('A monograph, edited volume or textbook.'),
  );
  $form['ingest_selector']['Newspaper or Magazine Article'] = array(
    '#description' => t('An article in a newspaper or magazine. Not peer-reviewed.'),
  );
  $form['ingest_selector']['Conference Item'] = array(
    '#description' => t('A paper, poster or presentation given at a conference. If the conference item has been published in a journal or book please use "Journal Article" or "Book Chapter" instead.'),
  );
  $form['ingest_selector']['Thesis'] = array(
    '#description' => t('A thesis or disseration.'),
  );
  $form['ingest_selector']['Patent'] = array(
    '#description' => t('A published patent.  Do not include as yet unpublished patent applications.'),
  );
  $form['ingest_selector']['Report'] = array(
    '#description' => t('A report or expert opinion.'),
  );

  // Handle using this form for editing when required, if the object is passed
  // then the form needs to add a submit button and use $form_state['object']
  // for processing.
  if ($editing) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Next'),
    );
  }
  else {
    // On ingest allow for DOI for Journal Articles.
    $form['journal_import_method'] = array(
      '#title' => t('Import Method'),
      '#type' => 'radios',
      '#options' => array(
        "manual" => t("Manually enter publication details"),
        "doi" => t("Import via DOI"),
      ),
      "#default_value" => $default['journal_import_method'],
      '#states' => array(
        'visible' => array(
          ':input[name="ingest_selector"]' => array('value' => 'Journal Article'),
        ),
      ),
    );
    $form['doi'] = array(
      '#title' => t('Digital Object Identifier'),
      '#type' => 'textfield',
      '#description' => t('Enter an identifier for which to attempt to acquire metadata, such as: 10.1016/j.jorganchem.2011.11.018, 10.1016/j.tiv.2011.10.017, 10.1111/j.1540-4560.2012.01733.x, http://dx.doi.org/10.1111/eva.12339, or http://doi.org/10.1111/eva.12340'),
      '#element_validate' => array('lib4ridora_doi_validate_id'),
      "#default_value" => $default['doi'],
      '#states' => array(
        'visible' => array(
          ':input[name="journal_import_method"]' => array('value' => 'doi'),
          ':input[name="ingest_selector"]' => array('value' => 'Journal Article'),
        ),
      ),
    );
  }
  return $form;
}

/**
 * Element validation callback; ensure the submitted DOI is valid.
 *
 * If it is valid, put it in the MODS datastream so it can be loaded in the
 * next step.
 */
function lib4ridora_doi_validate_id($element, &$form_state, $form) {
  if (isset($element['#value']) && $form_state['values']['journal_import_method'] == 'doi') {
    module_load_include('inc', 'islandora_doi', 'includes/utilities');
    $mods = islandora_doi_get_mods($element['#value']);
    if ($mods === FALSE) {
      form_error($element, t('"@value" does not appear to be a valid DOI.', array(
        '@value' => $element['#value'],
      )));
    }
    else {
      $object = islandora_ingest_form_get_object($form_state);
      if (isset($object["MODS"])) {
        $datastream = $object["MODS"];
        $datastream->setContentFromString(trim($mods->asXML()));
        if ($datastream->label != "MODS Record") {
          $datastream->label = "MODS Record";
        }
        if ($datastream->mimetype != 'text/xml') {
          $datastream->mimetype = 'text/xml';
        }
      }
      else {
        $datastream = $object->constructDatastream("MODS", 'M');
        $datastream->setContentFromString(trim($mods->asXML()));
        $datastream->label = "MODS Record";
        $datastream->mimetype = 'text/xml';
        $object->ingestDatastream($datastream);
      }
    }
  }
}

/**
 * Change the xml form association depending on the selector value.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function lib4ridora_ingest_selector_form_submit(array $form, array &$form_state) {
  module_load_include('inc', 'xml_form_builder', 'includes/associations');
  module_load_include('inc', 'lib4ridora', 'includes/utilities');
  $associations = xml_form_builder_get_associations();
  $association_step_storage = &islandora_ingest_form_get_step_storage($form_state, 'xml_form_builder_association_step');
  $association_step_storage['journal_import_method'] = $form_state['values']['journal_import_method'];
  $association_step_storage['doi'] = $form_state['values']['doi'];
  $association_step_storage['ingest_selector'] = $form_state['values']['ingest_selector'];
  $ingest_to_form_association = lib4ridora_get_custom_citation_form_associations();
  foreach ($associations as $key => $association) {
    if ($association['form_name'] == $ingest_to_form_association[$form_state['values']['ingest_selector']]) {
      // Update the content_model to be ir:citationCModel so that regardless of
      // the form used it will only have the ir:citationCModel.
      $association['content_model'] = "ir:citationCModel";
      $association_step_storage['association'] = $association;
      break;
    }
  }
}

/**
 * Undo's any changes the regular submit handler did to the association.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function lib4ridora_ingest_selector_form_undo_submit(array $form, array &$form_state) {
  $association_step_storage = &islandora_ingest_form_get_step_storage($form_state, 'xml_form_builder_association_step');
  unset($association_step_storage['association']);
  $object = islandora_ingest_form_get_object($form_state);
  if (isset($object["MODS"])) {
    unset($object["MODS"]);
  }
}
