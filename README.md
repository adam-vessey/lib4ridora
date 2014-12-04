# Lib4ridora

## Introduction

A module for specific customizations for the Library for the Research Institutes.

## Requirements

This module requires the following modules/libraries:

* [Islandora](https://github.com/islandora/islandora)
* [Islandora Scholar](https://github.com/islandora/islandora_scholar)
* [Islandora Plupload](https://github.com/discoverygarden/islandora_plupload)

## Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Usage

### XML Form Field Filtering

Fields in Islandora XML forms can be filtered by either role or permissions, by settings some properties in the "User Data" (under "More Advanced Controls", the third tab in the form builder; corresponds to `#user_data` in the form element array).

First, to activate this behaviour on an element (and all sub-elements), add `lib4ridora_field_filter`, with the value of `true`.

Then:
* To add a restriction to a single role to a field, add the property `lib4ridora_field_filter_role` on the particular field you wish to restrict. The value of the property must be the name of the role. To allow multiple roles, properties can be added prefixed with `lib4ridora_field_filter_role_` and ending with an arbitrary suffix. Note that properties on each field must be unique (attempting to specify multiple properties with the same name will result in only the last one being applied).
* To add a permission restriction, the same process as roles may be used, substituting the string "permission" for role: So properties named `lib4ridora_field_filter_permission` or prefixed with `lib4ridora_field_filter_permission_`, with the value being the name of the permission as might be passed to [`user_access()`](https://api.drupal.org/api/drupal/modules!user!user.module/function/user_access/7). Note that the Drupal's superuser (user '1') will always have permission when using permission restrictions.

Role and permission restrictions may be combined in the same element.

When restrictions are specified on an element, the element (and its children) will only be shown if the current user has one of the specified roles or one of the specified permissions.

For example, if one wanted to allow access to an element to those with the "Replace datastreams" permission or the "metadata manager" or "repository administrator" roles, your configuration might look like:
![Screenshot](http://puu.sh/dgOMH/ad3d3d7964.png)

## Maintainers/Sponsors

Current maintainers:

* [discoverygarden Inc.](http://github.com/discoverygarden)
