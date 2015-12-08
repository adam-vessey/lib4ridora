<?php
/**
 * @file
 * Admin form.
 */

/**
 * Admin for for lib4ridora.
 */
function lib4ridora_admin(array $form, array &$form_state) {

  $form['lib4ridora_issn_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('ISSN Solr Field'),
    '#default_value' => variable_get('lib4ridora_issn_solr_field', 'mods_identifier_issn_s'),
    '#required' => TRUE,
  );
  $form['lib4ridora_e_issn_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('E-ISSN Solr Field'),
    '#default_value' => variable_get('lib4ridora_e_issn_solr_field', 'mods_identifier_e-issn_s'),
    '#required' => TRUE,
  );

  $form['lib4ridora_solr_field_article_host_journal'] = array(
    '#type' => 'textfield',
    '#title' => t('Article Journal Solr Field'),
    '#description' => t('A Solr field on articles identifying to which journal it belongs. Should be the journal PID. (used in CSV export)'),
    '#default_value' => variable_get('lib4ridora_solr_field_article_host_journal', 'mods_relatedItem_host_identifier_ms'),
    '#required' => TRUE,
  );
  $form['lib4ridora_solr_field_article_date_issued'] = array(
    '#type' => 'textfield',
    '#title' => t('Article Date Issued Solr Field'),
    '#description' => t('A Solr field on articles containing (at least) the year in which the article was issued. (used in CSV export)'),
    '#default_value' => variable_get('lib4ridora_solr_field_article_date_issued', 'mods_originInfo_dateIssued_ms'),
    '#required' => TRUE,
  );
  $form['lib4ridora_pseudo_solr_field_factor'] = array(
    '#type' => 'textfield',
    '#title' => t('Journal Impact Factor Pseudo-Solr Field'),
    '#description' => t('A field we will inject into Solr documents during CSV exports. If restricting fields, ensure it is available.'),
    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_factor', 'journal_impact_factor'),
    '#required' => TRUE,
  );
  $form['lib4ridora_pseudo_solr_field_year'] = array(
    '#type' => 'textfield',
    '#title' => t('Journal Impact Factor Year Pseudo-Solr Field'),
    '#description' => t('A field we will inject into Solr documents during CSV exports. If restricting fields, ensure it is available.'),
    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_year', 'journal_impact_factor_year'),
    '#required' => TRUE,
  );

  return system_settings_form($form);
}

/**
 * Form building function; permit the selection of mappings.
 */
function lib4ridora_islandora_solr_metadata_mapping_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/utilities');
  form_load_include($form_state, 'inc', 'islandora_solr_metadata', 'includes/db');

  $form['#tree'] = TRUE;

  $raw_associations = islandora_solr_metadata_get_associations();
  $associations = array();
  foreach ($raw_associations as $association) {
    $associations[$association['machine_name']] = $association['name'];
  }
  $defaults = variable_get('lib4ridora_islandora_solr_metadata_mappings', array());

  $base_select = array(
    '#type' => 'select',
    '#options' => array(
      NULL => t('-- Default display --'),
    ) + $associations,
  );

  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Islandora Solr Metadata Mappings'),
    '#description' => t('Instead of using content models to choose which metadata display should be used, the mapping on this page can be used.'),
  );
  foreach (array_unique(lib4ridora_get_custom_citation_form_associations()) as $type) {
    $form['settings'][$type] = $base_select + array(
      '#title' => t('Mapping for @type', array(
        '@type' => $type,
      )),
      '#default_value' => (isset($defaults[$type]) && isset($associations[$defaults[$type]])) ?
        $defaults[$type] :
        NULL,
    );
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
function lib4ridora_islandora_solr_metadata_mapping_form_submit($form, $form_state) {
  variable_set('lib4ridora_islandora_solr_metadata_mappings', $form_state['values']['settings']);
}
