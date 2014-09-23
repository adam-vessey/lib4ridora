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

  return system_settings_form($form);
}
