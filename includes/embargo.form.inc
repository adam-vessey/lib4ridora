<?php

/**
 * @file
 * Ingest step form and friends.
 */

define('LIB4RIDORA_RELS_URI', ISLANDORA_RELS_INT_URI);

/**
 * Helper function to get the object during the form process.
 *
 * Gets either the object from ingest steps, or assumed the first parameter
 * passed to the form is an object, which we'll reload.
 */
function lib4ridora_multi_embargo_get_object($form_state) {
  list($object) = $form_state['build_info']['args'];
  if (!($object instanceof AbstractObject)) {
    module_load_include('inc', 'islandora', 'includes/ingest.form');
    $object = islandora_ingest_form_get_object($form_state);
  }
  else {
    // Reload, 'cause paranoia.
    $object = islandora_object_load($object->id);
  }

  return $object;
}

/**
 * Filter callback function.
 *
 * Matches datastreams we should deal with in this form... Both the "PDF"
 * and those starting with the "lib4ridora_extra_pdf_datastream_prefix" Drupal
 * variable.
 *
 * @param AbstractDatastream $datastream
 *   A datastream to test.
 *
 * @return bool
 *   TRUE if the datastream ID is "PDF" or starts with the string specified in
 *   the "lib4ridora_extra_pdf_datastream_prefix" Drupal variable; otherwise,
 *   FALSE.
 */
function lib4ridora_multi_embargo_pdf_filter(AbstractDatastream $datastream) {
  return $datastream->id == 'PDF' || strpos($datastream->id, variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF')) === 0;
}

/**
 * Form building function; build multi-PDF embargo form.
 */
function lib4ridora_multi_embargo_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/embargo.form');

  $object = lib4ridora_multi_embargo_get_object($form_state);

  $datastreams = iterator_to_array($object);
  $pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter');

  $form['#tree'] = TRUE;
  $form['datastreams'] = array(
    '#type' => 'markup',
  );

  foreach ($pdf_datastreams as $datastream) {
    $defaults = lib4ridora_get_embargo_info($datastream);
    $form['datastreams'][$datastream->id] = array(
      '#type' => 'fieldset',
      '#title' => t('@label (@id)', array(
        '@label' => $datastream->label,
        '@id' => $datastream->id,
      )),
      'document_version' => array(
        '#type' => 'select',
        '#title' => t('Document Version'),
        '#options' => array(
          'accepted version' => t('Accepted Version'),
          'published version' => t('Published Version'),
          'updated version' => t('Updated Version'),
          'supplemental material' => t('Supplemental Material'),
          'unspecified' => t('Unspecified'),
        ),
        '#default_value' => $defaults['document_version'],
      ),
      'availability' => array(
        '#type' => 'select',
        '#title' => t('Document Availablility'),
        '#options' => array(
          NULL => t('Public'),
          'intranet' => t('Intranet'),
          'date' => t('Embargo'),
          'private' => t('Private'),
        ),
        '#attributes' => array(
          'class' => array(
            'lib4ridora-embargo-select',
          ),
        ),
        '#default_value' => $defaults['embargo'],
      ),
      'embargo_date' => array(
        '#type' => 'date',
        '#title' => t('Embargo Date'),
        '#title_display' => 'invisible',
        '#states' => array(
          'visible' => array(
            'fieldset#edit-datastreams-' . strtolower($datastream->id) . ' select.lib4ridora-embargo-select' => array(
              'value' => 'date',
            ),
          ),
        ),
        '#default_value' => $defaults['embargo_date'],
      ),
      'use_permission' => array(
        '#type' => 'select',
        '#title' => t('Conditions for Reuse'),
        '#options' => array(
          NULL => t('Unspecified'),
          'CC BY' => t('Creative Commons: Attribution'),
          'CC BY-SA' => t('Creative Commons: Attribution-ShareAlike'),
          'CC BY-ND' => t('Creative Commons: Attribution-NoDerivs'),
          'CC BY-NC' => t('Creative Commons: Attribution-NonCommercial'),
          'CC BY-NC-SA' => t('Creative Commons: Attribution-NonCommercial-ShareAlike'),
          'CC BY-NC-ND' => t('Creative Commons: Attribution-NonCommercial-NoDerivs'),
        ),
        '#default_value' => $defaults['use_permission'],
      ),
    );
  }

  return $form;
}

/**
 * Get info about the datastream.
 *
 * @return array
 *   An associative array containing:
 *   - availability: Indicate if/how the datastream is available.
 *   - embargo_date: Array containing the 'year', 'month' and 'day'.
 *   - document_version
 *   - use_permission
 */
function lib4ridora_get_embargo_info(AbstractDatastream $datastream) {
  // Add in defaults.
  $to_return = lib4ridora_multi_pdf_defaults_properties();

  foreach ($to_return as $pred => $default) {
    $value = $datastream->relationships->get(LIB4RIDORA_RELS_URI, $pred);
    // Should be at most on value... Just get it.
    $value = reset($value);
    if ($value) {
      if ($key == 'embargo_date') {
        $to_return[$key] = array_intersect_key(date_parse($value['object']['value']), drupal_map_assoc(array(
          'year',
          'month',
          'day',
        )));
      }
      else {
        $to_return[$pred] = $value['object']['value'];
      }
    }
  }

  return $to_return;
}

/**
 * Form submission handler; apply multi-PDF embargoes and properties.
 */
function lib4ridora_multi_embargo_form_submit(&$form, &$form_state) {
  $object = lib4ridora_multi_embargo_get_object($form_state);
  foreach ($form_state['values']['datastreams'] as $dsid => $values) {
    $filtered = array_filter(array_intersect_key($values, lib4ridora_multi_pdf_defaults_properties()));
    if (isset($filtered['embargo_date']) && $filtered['availability'] == 'date') {
      $value = gmdate("Y-m-d\TH:i:s\Z", gmmktime(0, 0, 0, $values['embargo_date']['month'], $values['embargo_date']['day'], $values['embargo_date']['year']));
      lib4ridora_set_relationships($object[$dsid]->relationships, LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate('embargo_date'), $value, RELS_TYPE_DATETIME);
    }
    unset($filtered['embargo_date']);
    foreach ($filtered as $pred => $value) {
      lib4ridora_set_relationships($object[$dsid]->relationships, LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate($pred), $value, RELS_TYPE_PLAIN_LITERAL);
    }
  }
}

/**
 * Form "undo" submission handler; reverse application of multi-PDF embargoes.
 *
 * Also, get rid of some associated properties.
 */
function lib4ridora_multi_embargo_form_undo_submit(&$form, &$form_submit) {
  $object = lib4ridora_multi_embargo_get_object($form_state);
  $datastreams = iterator_to_array($object);
  $pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter');
  foreach ($pdf_datastreams as $pdf) {
    foreach (lib4ridora_multi_pdf_defaults_properties() as $pred => $default) {
      $pdf->relationships->remove(LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate($pred));
    }
  }
}

/**
 * Set a relationship.
 *
 * Add a relationship such that it is the only relationship with the given
 * predicate for the given subject.
 */
function lib4ridora_set_relationships(FedoraRelationships $rels, $pred_uri, $pred, $object, $object_type = RELS_TYPE_URI) {
  $rels->remove($pred_uri, $pred);
  $rels->add($pred_uri, $pred, $object, $object_type);
}

/**
 * Helper function to build up RDF predicates.
 *
 * @param string $base
 *   The unique part of the predicate.
 *
 * @return string
 *   A predicate which should be unique for our use.
 */
function lib4ridora_multi_embargo_build_predicate($base) {
  return "lib4ridora-multi-embargo-$base";
}

/**
 * Helper function, to enumerate the defaults.
 *
 * @return array
 *   An array of defaults for multi-pdf settings.
 */
function lib4ridora_multi_pdf_defaults_properties() {
  return array(
    'availability' => NULL,
    'embargo_date' => NULL,
    'document_version' => 'accepted version',
    'use_permission' => NULL,
  );
}
