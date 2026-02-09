"""
Parse Qt .ts translation files and generate a Translations.js file
for instant language switching in QML.
"""
import xml.etree.ElementTree as ET
import os
import json

def parse_ts_file(filepath):
    """Parse a Qt .ts file and extract all source->translation pairs."""
    translations = {}
    try:
        tree = ET.parse(filepath)
        root = tree.getroot()
        
        for context in root.findall('context'):
            for message in context.findall('message'):
                source_elem = message.find('source')
                translation_elem = message.find('translation')
                
                if source_elem is not None and source_elem.text:
                    source = source_elem.text
                    
                    # Skip vanished or unfinished translations
                    if translation_elem is not None:
                        trans_type = translation_elem.get('type', '')
                        if trans_type == 'vanished':
                            continue
                        
                        translation = translation_elem.text if translation_elem.text else source
                        
                        # Only add if translation is different from source
                        if translation and translation != source:
                            translations[source] = translation
    except Exception as e:
        print(f"Error parsing {filepath}: {e}")
    
    return translations

def generate_translations_js(translations_dir, output_file):
    """Generate the Translations.js file from all .ts files."""
    all_translations = {}
    
    # Language code mapping from filename
    lang_codes = []
    
    for filename in os.listdir(translations_dir):
        if filename.endswith('.ts'):
            # Extract language code from filename: AsusTufFanControl_ta.ts -> ta
            lang_code = filename.replace('AsusTufFanControl_', '').replace('.ts', '')
            filepath = os.path.join(translations_dir, filename)
            
            translations = parse_ts_file(filepath)
            if translations:
                all_translations[lang_code] = translations
                lang_codes.append(lang_code)
                print(f"Parsed {lang_code}: {len(translations)} translations")
    
    # Generate JavaScript file
    js_content = '''// Translations.js - Auto-generated from .ts files
// This provides instant language switching without app restart

.pragma library

var translations = {
    // English (default - no translation needed, serves as fallback)
    "en": {},
    
'''
    
    for lang_code in sorted(all_translations.keys()):
        trans = all_translations[lang_code]
        js_content += f'    // {lang_code.upper()}\n'
        js_content += f'    "{lang_code}": {{\n'
        
        for i, (source, translation) in enumerate(trans.items()):
            # Escape special characters for JavaScript
            source_escaped = source.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
            translation_escaped = translation.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
            
            comma = ',' if i < len(trans) - 1 else ''
            js_content += f'        "{source_escaped}": "{translation_escaped}"{comma}\n'
        
        js_content += '    },\n\n'
    
    js_content += '''};

// Translate function - instantly translates text based on current language
function tr(text, langCode) {
    if (!langCode || langCode === "en") {
        return text;
    }
    
    var langTranslations = translations[langCode];
    if (langTranslations && langTranslations[text]) {
        return langTranslations[text];
    }
    
    // Fallback to English
    return text;
}
'''
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(js_content)
    
    print(f"\nGenerated {output_file}")
    print(f"Total languages: {len(all_translations)}")

if __name__ == '__main__':
    translations_dir = r'E:\AsusTufFanControl_Windows\AsusTufFanControl_Windows\AsusController_Windows\translations'
    output_file = r'E:\AsusTufFanControl_Windows\AsusTufFanControl_Windows\AsusController_Windows\ui\Translations.js'
    
    generate_translations_js(translations_dir, output_file)
