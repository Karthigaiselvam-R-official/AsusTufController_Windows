import re
import os

def repair_ta_ts(file_path):
    if not os.path.exists(file_path):
        print(f"File not found: {file_path}")
        return

    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # --- Phase 1: Heavy Structural Normalization ---
    
    # 1. Fix broken entities immediately (before anything else)
    content = content.replace('& lt;', '&lt;')
    content = content.replace('& gt;', '&gt;')
    content = content.replace('type = "vanished"', 'type="vanished"')
    content = content.replace('encoding = \'utf-8\'', 'encoding="utf-8"')
    content = content.replace('<?xml version=\'1.0\'', '<?xml version="1.0"')

    # 2. De-duplicate: Remove ALL messages that contain a vanished translation tag
    messages = re.findall(r'<message[^>]*?>.*?</message>', content, re.DOTALL)
    for msg in messages:
        if 'type="vanished"' in msg:
            content = content.replace(msg, '')
    
    # 3. Collapse multiple whitespaces between tags (preserving content)
    content = re.sub(r'>\s+<', '><', content)
    
    # 4. Fix spaces inside tag brackets
    content = re.sub(r'<\s+', '<', content)
    content = re.sub(r'\s+>', '>', content)
    content = re.sub(r'<\s*/\s*', '</', content)
    content = re.sub(r'\s*/\s*>', '/>', content)
    
    # NEW step: Ensure location tags are clean
    content = re.sub(r'\s*=\s*"', '="', content)

    # --- Phase 2: Content-Specific Corrections ---
    
    # 5. Injection Logic: BatteryPage (Minimum 60%, Full (Limit), Plugged In)
    if '<name>BatteryPage</name>' in content:
        if '<source>Minimum 60%</source>' not in content:
            content = re.sub(r'(<name>BatteryPage</name>)', r'\1<message><source>Minimum 60%</source><translation>குறைந்தபட்சம் 60%</translation></message>', content)
        
        if '<source>Full (Limit)</source>' not in content:
            content = re.sub(r'(<name>BatteryPage</name>)', r'\1<message><source>Full (Limit)</source><translation>முழுமையானது (வரம்பு)</translation></message>', content)
            
        if '<source>Plugged In</source>' not in content:
            # We already have one in DashboardPage, let's inject specifically into BatteryPage if missing
            content = re.sub(r'(<name>BatteryPage</name>)', r'\1<message><source>Plugged In</source><translation>செருகப்பட்டது</translation></message>', content)

    # 6. Injection Logic: SettingsPage (Auto)
    if '<name>SettingsPage</name>' in content and '<source>Auto</source>' not in content:
        # Insert after the name tag of SettingsPage
        content = re.sub(r'(<name>SettingsPage</name>)', r'\1<message><source>Auto</source><translation>தானியங்கி</translation></message>', content)

    # 7. Surgical corrections for specific strings (preserving previous fixes as requested)
    
    # Strip whitespace inside tags first for normalization
    content = re.sub(r'<source>\s*(.*?)\s*</source>', r'<source>\1</source>', content)
    content = re.sub(r'<translation(.*?)>\s*(.*?)\s*</translation>', r'<translation\1>\2</translation>', content)
    
    # (OVERDRIVE) - Must have leading space
    content = content.replace('<source>(OVERDRIVE)</source>', '<source> (OVERDRIVE)</source>')
    content = content.replace('<translation>(மிதமிஞ்சிய வேகம்)</translation>', '<translation> (மிதமிஞ்சிய வேகம்)</translation>')
    
    # Temperature Units - Maintain exact spacing
    content = content.replace('<source>Celsius(°C)</source>', '<source>Celsius (°C)</source>')
    content = content.replace('<source>Fahrenheit(°F)</source>', '<source>Fahrenheit (°F)</source>')
    content = content.replace('<translation>செல்சியஸ்(°C)</translation>', '<translation>செல்சியஸ் (°C)</translation>')
    content = content.replace('<translation>ஃபாரன்ஹீட்(°F)</translation>', '<translation>ஃபாரன்ஹீட் (°F)</translation>')
    
    # Fan Section (Legend) - Note trailing spaces in source AND translation
    content = content.replace('<source>Silent &lt;</source>', '<source>Silent &lt; </source>')
    content = content.replace('<translation>அமைதி &lt;</translation>', '<translation>அமைதி &lt; </translation>')
    content = content.replace('<source>Turbo &gt;</source>', '<source>Turbo &gt; </source>')
    content = content.replace('<translation>டர்போ &gt;</translation>', '<translation>டர்போ &gt; </translation>')
    
    # Balanced variants logic:
    # We normalized tags already, so we can use a precise match for the legend variant
    content = content.replace('<location filename="../ui/pages/FanPage.qml" line="1125"/><source>Balanced</source>', '<location filename="../ui/pages/FanPage.qml" line="1125"/><source>Balanced </source>')
    content = content.replace('<location filename="../ui/pages/FanPage.qml" line="1125"/><source>Balanced </source><translation>சமநிலை</translation>', '<location filename="../ui/pages/FanPage.qml" line="1125"/><source>Balanced </source><translation>சமநிலை </translation>')

    # Fan Section (Modes) - Ensure spaces before parentheses
    content = content.replace('<source>Silent Mode(below)</source>', '<source>Silent Mode (below)</source>')
    content = content.replace('<source>Turbo Mode(above)</source>', '<source>Turbo Mode (above)</source>')
    content = content.replace('<translation>அமைதி முறை(கீழே)</translation>', '<translation>அமைதி முறை (கீழே)</translation>')
    content = content.replace('<translation>டர்போ முறை(மேலே)</translation>', '<translation>டர்போ முறை (மேலே)</translation>')
    
    # Battery Section % formatting
    content = content.replace('Maximum 100 %', 'Maximum 100%')
    content = content.replace('Minimum 60 %', 'Minimum 60%')
    content = content.replace('அதிகபட்சம் 100 %', 'அதிகபட்சம் 100%')
    content = content.replace('குறைந்தபட்சம் 60 %', 'குறைந்தபட்சம் 60%')

    
    # --- Phase 2.1: Final Cleanup Catch-all ---
    # Fix common spacing issues in translations
    content = content.replace('செல்சியஸ்(°C)', 'செல்சியஸ் (°C)')
    content = content.replace('ஃபாரன்ஹீட்(°F)', 'ஃபாரன்ஹீட் (°F)')
    content = content.replace('அமைதி முறை(கீழே)', 'அமைதி முறை (கீழே)')
    content = content.replace('டர்போ முறை(மேலே)', 'டர்போ முறை (மேலே)')
    
    # Catch cases where they were already stripped but missing the space in source
    content = content.replace('Celsius(°C)', 'Celsius (°C)')
    content = content.replace('Fahrenheit(°F)', 'Fahrenheit (°F)')
    content = content.replace('Silent Mode(below)', 'Silent Mode (below)')
    content = content.replace('Turbo Mode(above)', 'Turbo Mode (above)')
    
    # --- Phase 3: Pretty Printing (Indentation) ---
    # We want a clean 4-space indent.
    
    indent = 0
    fixed_lines = []
    
    # Split the long string into tags and their content
    # This regex splits by tags while keeping the tags.
    parts = re.split(r'(<[^>]+?>)', content)
    
    for part in parts:
        if not part: continue
        
        # If it's a closing tag
        if part.startswith('</'):
            indent = max(0, indent - 1)
            fixed_lines.append('    ' * indent + part)
            continue
            
        # If it's a processing instruction or the TS declaration
        if part.startswith('<?xml') or part.startswith('<TS'):
            fixed_lines.append(part)
            if part.startswith('<TS'): indent += 1
            continue
            
        # If it's an opening tag
        if part.startswith('<') and not part.endswith('/>'):
            # If it's a "leaf" tag like <source>, <translation>, <name>, <location>
            # we want to try to keep it and its content on the same line if possible,
            # but for simplicity in a robust repair, we can just indent.
            # However, <source> and <translation> should usually wrap their text.
            
            fixed_lines.append('    ' * indent + part)
            # Only increment indent if it's NOT a known leaf-like tag (though TS isn't strictly leaf-based)
            # Actually, context and message are the only ones that really need nested indentation.
            if part.startswith('<context>') or part.startswith('<message>'):
                indent += 1
            continue
            
        # If it's a self-closing tag
        if part.endswith('/>'):
            fixed_lines.append('    ' * indent + part)
            continue
            
        # It's content between tags
        # Append it to the previous line (the opening tag) to keep them together if they are small tags
        if fixed_lines and fixed_lines[-1].strip().startswith('<') and not fixed_lines[-1].strip().startswith('</'):
            fixed_lines[-1] += part
        else:
            fixed_lines.append('    ' * indent + part)

    # Final cleanup: if a line ends with a tag but doesn't have its closing tag on same line,
    # and the NEXT part is the closing tag, join them.
    # This is to get <source>Text</source> on one line.
    
    final_output = []
    i = 0
    lines = fixed_lines
    while i < len(lines):
        line = lines[i]
        if i + 1 < len(lines):
            next_line = lines[i+1].strip()
            if next_line.startswith('</') and line.strip().startswith('<') and not line.strip().startswith('</'):
                # Check if tags match
                tag_name = re.search(r'<([a-zA-Z]+)', line).group(1)
                closing_tag_name = re.search(r'</([a-zA-Z]+)', next_line).group(1)
                if tag_name == closing_tag_name:
                    final_output.append(line + next_line)
                    i += 2
                    continue
        final_output.append(line)
        i += 1

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write("\n".join(final_output) + "\n")

if __name__ == "__main__":
    repair_ta_ts("translations/AsusTufFanControl_ta.ts")
    print("Full Structural and Content Repair Complete.")
