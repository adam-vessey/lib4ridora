<?php

/**
 * @file
 * Map uploaded datastream to file step form.
 */

/**
 * Form building function.
 */
function lib4ridora_datastream_select_form($form, &$form_state) {
  $upload_step_storage =& islandora_ingest_form_get_step_storage($form_state, 'lib4ridora_multi_pdf_upload');
  $files = $upload_step_storage['files'];
  $step_storage =& islandora_ingest_form_get_step_storage($form_state);
  $file_options = array();
  foreach ($files as $file) {
    $file_options["{$file->filename}--{$file->uid}--PDF"] = array(
      'info' => $file,
      'filename' => $file->filename,
    );
  }
  $step_storage['options'] = $file_options;

  reset($main_options);
  $form['info'] = array(
    '#type' => 'item',
    '#title' => t('Select the primary datastream'),
    '#description' => t('Select the datastream from which display derivatives will be created. All other files will be added as supplementary datastreams.'),
  );
  $form['select'] = array(
    '#type' => 'tableselect',
    '#title' => t('Select the primary asset'),
    '#description' => t('Select the asset from which display derivatives will be created.'),
    '#multiple' => FALSE,
    '#header' => array(
      'filename' => t('File name'),
    ),
    '#options' => $file_options,
    '#default_value' => key($file_options),
  );

  return $form;
}

/**
 * Form "undo" submission handler.
 */
function lib4ridora_datastream_select_form_undo_submit(&$form, &$form_state) {
  // Delete the main datastream from the object.
  // Delete the other datastreams from the object.
  $step_storage =& islandora_ingest_form_get_step_storage($form_state);
  $object = islandora_ingest_form_get_object($form_state);
  foreach ($step_storage['datastreams'] as $id => $file) {
    $object->purgeDatastream($id);
  }
  $step_storage['datastreams'] = array();
}

/**
 * Form validation handler.
 */
function lib4ridora_datastream_select_form_validate(&$form, &$form_state) {
  $selected = drupal_array_get_nested_value($form_state['values'], array('select'));
  if (empty($selected)) {
    form_set_error('', t('You must select a file for the primary datastream.'));
  }
}

/**
 * Form submission handler.
 */
function lib4ridora_datastream_select_form_submit(&$form, &$form_state) {
  $step_storage =& islandora_ingest_form_get_step_storage($form_state);
  $upload_step_storage =& islandora_ingest_form_get_step_storage($form_state, 'lib4ridora_multi_pdf_upload');
  $files = $upload_step_storage['files'];
  $step_storage['datastreams'] = array();

  // Throw the selected file onto the main PDF datastream.
  $selected = drupal_array_get_nested_value($form_state['values'], array('select'));
  $info = $step_storage['options'][$selected]['info'];

  $object = islandora_ingest_form_get_object($form_state);
  $datastream = $object->constructDatastream('PDF', 'M');
  $datastream->mimetype = $info->filemime;
  $datastream->label = $info->filename;
  $datastream->setContentFromFile($info->uri, FALSE);
  $object->ingestDatastream($datastream);
  unset($files[$info->filename]);
  $step_storage['datastreams']['PDF'] = $info;

  // Throw all the other uploaded files an incremented PDF datastream.
  foreach (array_values($files) as $index => $file) {
    $datastream_id = format_string('@prefix@index', array(
      '@prefix' => variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF'),
      '@index' => $index + 2,
    ));

    $datastream = $object->constructDatastream($datastream_id, 'M');
    $datastream->mimetype = $file->filemime;
    $datastream->label = $file->filename;
    $datastream->setContentFromFile($file->uri, FALSE);
    $object->ingestDatastream($datastream);
    $step_storage['datastreams'][$datastream_id] = $file;
  }
}
