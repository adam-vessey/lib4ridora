<?php

/**
 * @file
 * Ingest step form and friends.
 */

define('LIB4RIDORA_VERSION_PRED', 'lib4ridora-document-version');
define('LIB4RIDORA_PERM_PRED', 'lib4ridora-use-permission');

/**
 * Form building function; build multi-PDF embargo form.
 */
function lib4ridora_multi_embargo_form($form, &$form_state, AbstractObject $object = NULL) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/embargo.form');

  if ($object === NULL) {
    module_load_include('inc', 'islandora', 'includes/ingest.form');
    $object = islandora_ingest_form_get_object($form_state);
  }
  $form_state['lib4ridora']['object'] = $object;
  $datastreams = iterator_to_array($object);
  $pdf_filter = function ($datastream) {
    return strpos($datastream->id, 'PDF') === 0;
  };
  $pdf_datastreams = array_filter($datastreams, $pdf_filter);

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
      'embargo' => array(
        '#type' => 'select',
        '#title' => t('Embargo'),
        '#options' => array(
          NULL => t('None'),
          'indefinite' => t('Embargo indefinitely'),
          'date' => t('Set the embargo date'),
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
            'fieldset#edit-' . strtolower($datastream->id) . '-fieldset select.lib4ridora-embargo-select' => array(
              'value' => 'date',
            ),
          ),
        ),
        '#default_value' => $defaults['embargo_date'],
      ),
      'document_version' => array(
        '#type' => 'select',
        '#title' => t('Document Version'),
        '#options' => array(
          'pre-publication' => t('Pre-publication'),
          'published' => t('Published'),
        ),
        '#default_value' => $defaults['document_version'],
      ),
      'use_permission' => array(
        '#type' => 'select',
        '#title' => t('User Permission'),
        '#options' => array(
          'contact-publisher' => t('Contact Publisher (I do not hold copyright)'),
          'contact-author' => t('Contact Author (I hold copyright)'),
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
 *   - embargo: Indicate if/how the datastream is embargoed.
 *   - embargo_date: Only if "embargo" is "date". The ISO8601 timestamp.
 *   - document_version
 *   - use_permission
 */
function lib4ridora_get_embargo_info(AbstractDatastream $datastream) {
  $to_return = array(
    'embargo' => NULL,
    'embargo_date' => NULL,
    'document_version' => 'pre-published',
    'use_permission' => 'contact-author',
  );

  $rels = $datastream->relationships;
  $until = $rels->get(ISLANDORA_SCHOLAR_EMBARGO_RELS_URI, ISLANDORA_SCHOLAR_EMBARGO_EXPIRY_PRED);
  $until = reset($until);

  if ($until) {
    $value = $until['object']['value'];

    $to_return['embargo'] = ($value == 'indefinite' ?
      $value :
      'date');
    if ($value != 'indefinite') {
      $to_return['embargo_date'] = array_intersect_key(date_parse($value), drupal_map_assoc(array(
        'year',
        'month',
        'day',
      )));
    }
  }

  $version = $rels->get(ISLANDORA_SCHOLAR_EMBARGO_RELS_URI, LIB4RIDORA_VERSION_PRED);
  $version = reset($version);
  if ($version) {
    $to_return['document_version'] = $version['object']['value'];
  }

  $perm = $rels->get(ISLANDORA_SCHOLAR_EMBARGO_RELS_URI, LIB4RIDORA_PERM_PRED);
  $perm = reset($perm);
  if ($perm) {
    $to_return['use_permission'] = $perm['object']['value'];
  }

  return $to_return;
}

/**
 * Form submission handler; apply multi-PDF embargoes and properties.
 */
function lib4ridora_multi_embargo_form_submit(&$form, &$form_state) {
  $object = $form_state['lib4ridora']['object'];
  if ($object instanceof FedoraObject) {
    // Reload existing FedoraObject's, to avoid potential caching issues,
    // if/when this form gets used for editing.
    $object = islandora_object_load($object->id);
  }

  foreach ($form_state['values']['datastreams'] as $dsid => $values) {
    if ($values['embargo'] == 'indefinite') {
      islandora_scholar_embargo_set_embargo($object, $dsid, 'indefinite');
    }
    elseif ($values['embargo'] == 'date') {
      islandora_scholar_embargo_set_embargo($object, $dsid, gmdate("Y-m-d\TH:i:s\Z", gmmktime(0, 0, 0, $values['embargo_date']['day'], $values['embargo_date']['month'], $values['embargo_date']['year'])));
    }

    lib4ridora_set_relationships($object[$dsid]->relationships, ISLANDORA_SCHOLAR_EMBARGO_RELS_URI, LIB4RIDORA_VERSION_PRED, $values['document_version']);
    lib4ridora_set_relationships($object[$dsid]->relationships, ISLANDORA_SCHOLAR_EMBARGO_RELS_URI, LIB4RIDORA_PERM_PRED, $values['use_permission']);
  }
}

/**
 * Form "undo" submission handler; reverse application of multi-PDF embargoes.
 *
 * Also, get rid of some associated properties.
 */
function lib4ridora_multi_embargo_form_undo_submit(&$form, &$form_submit) {
  $object = $form_state['lib4ridora']['object'];
  $datastreams = iterator_to_array($object);
  $pdf_filter = function ($datastream) {
    return strpos($datastream->id, 'PDF') === 0;
  };
  $pdf_datastreams = array_filter($datastreams, $pdf_filter);
  foreach ($pdf_datastreams as $pdf) {
    islandora_scholar_embargo_lift_embargo($object, $pdf->id);
    $pdf->relationships->remove(ISLANDORA_SCHOLAR_EMBARGO_RELS_URI, LIB4RIDORA_VERSION_PRED);
    $pdf->relationships->remove(ISLANDORA_SCHOLAR_EMBARGO_RELS_URI, LIB4RIDORA_PERM_PRED);
  }
}

/**
 * Set a relationship.
 *
 * Add a relationship such that it is the only relationship with the given
 * predicate for the given subject.
 *
 *
 */
function lib4ridora_set_relationships(FedoraRelationships $rels, $pred_uri, $pred, $object, $object_type = RELS_TYPE_URI) {
  $rels->remove($pred_uri, $pred);
  $rels->add($pred_uri, $pred, $object, $object_type);
}
