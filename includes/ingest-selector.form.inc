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
 * @param AbstractObject $object
 *   The object if the form is coming from editing.
 *
 * @return array
 *   The Drupal form state.
 */
function lib4ridora_ingest_selector_form(array $form, array &$form_state, AbstractObject $object = NULL) {
  $form['ingest_selector'] = array(
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
    "#default_value" => "Journal Article",
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
  $form_state['object'] = $object;
  if ($object) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Next',
    );
  }
  return $form;
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
  if ($form_state['object']) {
    $association_step_storage = array();
  }
  else {
    $association_step_storage = &islandora_ingest_form_get_step_storage($form_state, 'xml_form_builder_association_step');
  }
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
  if ($form_state['object']) {
    session_start();
    $_SESSION['association'] = $association_step_storage['association'];
    drupal_goto("islandora/object/{$form_state['object']->id}/MODS/citation/unknown");
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
}
