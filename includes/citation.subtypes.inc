<?php
/**
 * @file
 * Helper functions to deal with ir:citationCModel subtypes.
 */

/**
 * Get information about the different subtypes.
 *
 * @return array
 *   An associative array mapping a stable machine name to an associative array
 *   containing:
 *   - name: A human-readable name for the given type.
 *   - description: A verbose description of the given type.
 *   - genres: An array of genres which map to the given type.
 *   - form: The name of the form to use for the given type. NOTE: May be NULL
 *     if there is none mapped for the given type.
 *   - display: The machine name of the Solr metadata display to use for the
 *     given type. NOTE: May be NULL if there is none mapped for the given
 *     type.
 */
function lib4ridora_citation_subtypes() {
  static $map = NULL;
  if ($map === NULL) {
    $map = array(
      'journal_article' => array(
        'name' => t('Journal Article'),
        'description' => t('An article in a scientific journal.  Not necessarily peer-reviewed. May be an electronic-only medium such as online journal.'),
      ),
      'book_chapter' => array(
        'name' => t('Book Chapter'),
        'description' => t('A chapter or contribution in a book.'),
      ),
      'book' => array(
        'name' => t('Book'),
        'description' => t('A monograph, edited volume or textbook.'),
      ),
      'newspaper_or_magazine_article' => array(
        'name' => t('Newspaper or Magazine Article'),
        'description' => t('An article in a newspaper or magazine. Not peer-reviewed.'),
      ),
      'conference_object' => array(
        'name' => t('Conference Item'),
        'description' => t('A paper, poster or presentation given at a conference. If the conference item has been published in a journal or book please use "Journal Article" or "Book Chapter" instead.'),
      ),
      'thesis' => array(
        'name' => t('Thesis'),
        'description' => t('A thesis or disseration.'),
      ),
      'patent' => array(
        'name' => t('Patent'),
        'description' => t('A published patent.  Do not include as yet unpublished patent applications.'),
      ),
      'report' => array(
        'name' => t('Report'),
        'description' => t('A report or expert opinion.'),
      ),
    );

    // Map MODS genres to our machine names...
    $genre_map = array(
      'Journal Article' => 'journal_article',
      'Book Chapter' => 'book_chapter',
      'Book' => 'book',
      'Edited Book' => 'book',
      'Newspaper or Magazine Article' => 'newspaper_or_magazine_article',
      'Conference Item' => 'conference_object',
      'Proceedings Paper' => 'conference_object',
      'Other Conference Item' => 'conference_object',
      'Report' => 'report',
      'Thesis' => 'thesis',
      'dissertation' => 'thesis',
      'master thesis' => 'thesis',
      'bachelor thesis' => 'thesis',
      'Patent' => 'patent',
    );
    $form_map = variable_get('lib4ridora_citation_subtype_form_associations', array());
    $display_map = variable_get('lib4ridora_islandora_solr_metadata_mappings', array());
    foreach ($map as $name => &$info) {
      $info['genres'] = array_keys($genre_map, $name);
      $info['form'] = isset($form_map[$name]) ? $form_map[$name] : NULL;
      $info['display'] = isset($display_map[$name]) ? $display_map[$name] : NULL;
    }
    // XXX: Deal with leaked reference.
    unset($info);
  }

  return $map;
}

function lib4ridora_citation_subtype_islandora_solr_metadata_options() {
  module_load_include('inc', 'islandora_solr_metadata', 'includes/db');
  $raw_associations = islandora_solr_metadata_get_associations();
  $associations = array();
  foreach ($raw_associations as $association) {
    $associations[$association['machine_name']] = $association['name'];
  }
  return $associations;
}

function lib4ridora_citation_subtype_form_options() {
  return array();
}

/**
 * Form building function; permit the selection of mappings.
 *
 * @param string $variable
 *   The name of the Drupal variable in which to store values.
 * @param array $strings
 *   An associative array mapping a few string for use in the display, including:
 *   - title: Title to use near the top of the form.
 *   - description: Description to use near the top of the form.
 *   - mapping_title: Title for each type, with some substitutions available:
 *     - '@type': The human-readable name of the given subtype.
 *   - submit: The text for the submit button.
 */
function lib4ridora_citation_subtype_mapping_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/utilities');
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/citation.subtypes');

  $form['#tree'] = TRUE;
  $form['subtype_mappings'] = array(
    '#type' => 'vertical_tabs',
  );

  $variables = array(
    'lib4ridora_islandora_solr_metadata_mappings' => array(
      'options' => lib4ridora_citation_subtype_islandora_solr_metadata_options(),
      'strings' => array(
        'title' => t('Metadata Display'),
        'description' => t('Instead of using content models to choose which metadata display should be used, the mapping on this page can be used.'),
        'mapping_title' => t('Mapping for @type'),
      ),
    ),
    'lib4ridora_citation_subtype_form_associations' => array(
      'options' => lib4ridora_citation_subtype_form_options(),
      'strings' => array(
        'title' => t('XML Form'),
        'description' => t('Instead of using content models to choose which XML form should be used, the mapping on this page can be used.'),
        'mapping_title' => t('Mapping for @type'),
      ),
    ),
  );
  foreach ($variables as $variable => $info) {
    $defaults = variable_get($variable, array());

    $base_select = array(
      '#type' => 'select',
      '#options' => array(
        NULL => t('-- Default/None --'),
      ) + $info['options'],
    );

    $form['settings'][$variable] = array(
      '#type' => 'fieldset',
      '#group' => 'subtype_mappings',
      '#title' => $info['strings']['title'],
      '#description' => $info['strings']['description'],
    );
    foreach (lib4ridora_citation_subtypes() as $type => $type_info) {
      $form['settings'][$variable][$type] = $base_select + array(
        '#title' => format_string($info['strings']['mapping_title'], array(
          '@type' => $type_info['name'],
        )),
        '#description' => $type_info['description'],
        '#default_value' => ((isset($defaults[$type]) && isset($info['options'][$defaults[$type]])) ?
          $defaults[$type] :
          NULL),
      );
    }
  }

  $form['actions'] = array(
    '#type' => 'actions',
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit mappings'),
    ),
  );

  return $form;
}

/**
 * Form submission handler; persist the mapping selection.
 */
function lib4ridora_citation_subtype_mapping_form_submit(&$form, &$form_state) {
  foreach ($form_state['values']['settings'] as $variable => $values) {
    variable_set($variable, $values);
  }
  drupal_set_message(t('Saved mappings.'));
}
