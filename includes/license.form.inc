<?php

/**
 * @file
 * Ingest step form for licensing.
 */

/**
 * License form.
 */
function lib4ridora_license_form($form, &$form_state) {
  $form['license'] = array(
    '#type' => 'item',
    '#title' => t('Repository Deposit License'),
    '#description' => t('Lorem ipsum dolor sit amet, consectetur adipisicing
      elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
      Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut
      aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in
      voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur
      sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt
      mollit anim id est laborum.'),
  );
  return $form;
}
