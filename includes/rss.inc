<?php

function lib4ridora_mods_for_rss(DOMDocument $doc) {
  $xslt_processor = new XSLTProcessor();
  $xslt_doc = new DOMDocument();
  $xslt_doc->load(drupal_get_path('module', 'lib4ridora') . '/xml/mods-pare.xslt');
  $xslt_processor->importStylesheet($xslt_doc);
  $transformed = $xslt_processor->transformToDoc($doc);
  //dsm($transformed->saveXML());

  $parser = new Lib4ridoraDOMElementToFormattableArrayParser($transformed->documentElement);
  return $parser->getParsedArray();
}

class Lib4ridoraDOMElementToFormattableArrayParser {
  const MODS_NAMESPACE = 'http://www.loc.gov/mods/v3';
  protected $root;
  public function __construct(DOMElement $root) {
    $this->root = $root;
  }

  protected function parseAttributes(DOMNamedNodeMap $attributes) {
    $to_return = array();

    foreach ($attributes as $attribute) {
      if ($attribute->namespaceURI) {
        $name = format_string('!prefix:!name', array(
          '!prefix' => $attribute->lookupPrefix($attribute->namespaceURI),
          '!name' => $attribute->localName,
        ));
      }
      else {
        $name = $attribute->localName;
      }

      $to_return[$name] = $attribute->nodeValue;
    }

    return $to_return;
  }

  protected function filterToElements(DOMNode $node) {
    return $node instanceof DOMElement;
  }

  // TODO: Need better namespace handling.
  protected function parse(DOMElement $element) {
    dsm($element->nodeName);
    return array(
      'key' => $element->namespaceURI == static::MODS_NAMESPACE ?
        "mods:{$element->localName}" :
        $element->lookupPrefix($element->namespaceURI) . ':' . $element->localName,
      'value' => $element->hasChildNodes() ?
        array_map(array($this, 'parse'), array_filter(iterator_to_array($element->childNodes), array($this, 'filterToElements'))) :
        $element->nodeValue,
      'attributes' => $this->parseAttributes($element->attributes),
    );
  }

  public function getParsedArray() {
    $to_return = $this->parse($this->root);

    if ($to_return['key'] == 'mods:mods' && !isset($to_return['attributes']['xmlns:mods'])) {
      $to_return['attributes']['xmlns:mods'] = 'http://www.loc.gov/mods/v3';
    }

    return $to_return;
  }
}
