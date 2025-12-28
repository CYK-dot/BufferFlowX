#!/usr/bin/env python3
"""
BufferFlowX LD Script Generator
This script generates a final LD script by combining region information,
section information, and a template LD file.
"""

import argparse
import json
import os
import sys
from jinja2 import Environment, FileSystemLoader
from log_utils import log_info, log_error, log_success, exit_with_error


def load_json_file(filepath):
    """Load JSON file and return the content."""
    with open(filepath, 'r', encoding='utf-8') as f:
        return json.load(f)


def validate_regions_in_sections(sections_data, regions_data):
    """Validate that all regions referenced in sections exist in regions data."""
    regions_map = {region['name']: region for region in regions_data}
    
    for section in sections_data:
        region_name = section.get('region', '')
        if region_name not in regions_map:
            raise ValueError(f"Region '{region_name}' referenced in section '{section.get('name', 'unknown')}' not found in regions data")


def validate_sections_block(ld_content):
    """Validate that the LD content contains a SECTIONS block."""
    if 'SECTIONS' not in ld_content:
        raise ValueError("LD template does not contain a SECTIONS block")


def generate_copy_sections_c_file(copy_sections, output_c_file):
    """Generate a C file with constructors to copy sections from FLASH to RAM before main()."""
    if not copy_sections:
        return

    # Load the Jinja2 template for C file
    template_dir = os.path.dirname(__file__)
    env = Environment(loader=FileSystemLoader(template_dir), trim_blocks=True, lstrip_blocks=True)
    template = env.get_template('copy_sections_template.j2')
    
    # Render the C file content using the template
    c_content = template.render(sections=copy_sections)

    # Write the C file
    with open(output_c_file, 'w', encoding='utf-8') as f:
        f.write(c_content)


def generate_ld_script(regions_file, sections_file, template_file, output_file, copy_sections_file=None, output_c_file=None):
    """Generate the final LD script."""
    # Load all required files
    regions_data = load_json_file(regions_file)
    sections_data = load_json_file(sections_file)
    
    with open(template_file, 'r', encoding='utf-8') as f:
        ld_template_content = f.read()
    
    # Validate the inputs
    # We need to extract the sections from the sections.json structure
    all_sections = []
    if 'sections' in sections_data:
        for component_name, component_sections in sections_data['sections'].items():
            for section in component_sections:
                section_entry = {
                    'source_file': section.get('source_file', ''),
                    'component_name': component_name,
                    'section_name': section.get('section_name', ''),
                    'region': section.get('region', ''),
                    'load_region': section.get('load_region', ''),  # New field for RAM AT FLASH
                    'align': section.get('align', 0),
                    'address': section.get('address', 0),
                    'size': section.get('size', 0)
                }
                all_sections.append(section_entry)
    
    # Validate that all regions referenced in sections exist in regions data
    regions_map = {region_name: region for region_name, region in regions_data.items()}
    for section in all_sections:
        region_name = section['region']
        if region_name not in regions_map:
            raise ValueError(f"Region '{region_name}' referenced in section '{section['component_name']}.{section['section_name']}' not found in regions data")
        
        # Also validate load_region if it exists
        if section['load_region'] and section['load_region'] not in regions_map:
            raise ValueError(f"Load region '{section['load_region']}' referenced in section '{section['component_name']}.{section['section_name']}' not found in regions data")
    
    validate_sections_block(ld_template_content)
    
    # Prepare section data for the template
    processed_sections = []
    for section in all_sections:
        component_name = section['component_name']
        section_name = section['section_name']
        region_name = section['region']
        load_region = section['load_region']  # New field for RAM AT FLASH
        size = section['size']
        align = section['align']
        
        # Create section entry with the naming convention: .component.section
        # From the sections.json, the section_name already includes the dot prefix (e.g., ".test_section")
        # So we'll use the component name as prefix: .component_name.section_name (e.g., .test_component.test_section)
        full_section_name = f".{component_name}{section_name}"  # This creates .test_component.test_section
        
        # Create the section entry with proper LD syntax
        # Include align and size specifications, and generate start/end symbols
        clean_section_name = full_section_name.replace('.', '_').replace('-', '_')
        
        processed_section = {
            'full_section_name': full_section_name,
            'region_name': region_name,
            'load_region': load_region,  # Pass the load_region to the template
            'size': size,
            'align': align,
            'clean_section_name': clean_section_name
        }
        processed_sections.append(processed_section)
    
    # Load the Jinja2 template
    template_dir = os.path.dirname(__file__)
    env = Environment(loader=FileSystemLoader(template_dir), trim_blocks=True, lstrip_blocks=True)
    template = env.get_template('sections_template.j2')
    
    # Render the sections content using the template
    rendered_sections = template.render(sections=processed_sections)
    
    # Insert the generated sections into the LD template
    # Find the SECTIONS block and insert the new sections before the closing brace
    lines = ld_template_content.split('\n')
    
    # Find the SECTIONS block start
    sections_start_idx = -1
    for i, line in enumerate(lines):
        if 'SECTIONS' in line and '{' in line:
            sections_start_idx = i
            break
    
    # If we didn't find SECTIONS with {, look for SECTIONS line and the next {
    if sections_start_idx == -1:
        for i, line in enumerate(lines):
            if 'SECTIONS' in line:
                sections_start_idx = i
                # Find the opening brace for SECTIONS
                for j in range(i, len(lines)):
                    if '{' in lines[j]:
                        sections_start_idx = j
                        break
                break
    
    if sections_start_idx == -1:
        raise ValueError("Could not find SECTIONS block in the template")
    
    # Find the end of the SECTIONS block by counting braces properly
    brace_count = 0
    sections_end_idx = -1
    
    for i, line in enumerate(lines):
        # Process each character in the line to properly count braces
        for char in line:
            if char == '{':
                brace_count += 1
            elif char == '}':
                brace_count -= 1
                # If we're inside the SECTIONS block and braces are balanced, this is the end
                if brace_count == 0 and i >= sections_start_idx:
                    sections_end_idx = i
                    break
        if sections_end_idx != -1:
            break
    
    if sections_end_idx == -1:
        raise ValueError("Could not find end of SECTIONS block in the template")
    
    # Insert the generated sections before the closing brace of SECTIONS
    final_lines = lines[:sections_end_idx]
    # Add a blank line and the new sections before the closing brace
    final_lines.extend([''] + rendered_sections.split('\n') + [''])
    final_lines.extend(lines[sections_end_idx:])
    
    # Write the final LD script
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(final_lines))
    
    # If copy_sections_file is specified, we need to collect sections that have load_region specified
    copy_sections = [section for section in processed_sections if section['load_region']]
    if copy_sections_file:
        if copy_sections:
            with open(copy_sections_file, 'w', encoding='utf-8') as f:
                json.dump(copy_sections, f, indent=2)
    
    # If output_c_file is specified and there are sections that need copying, generate the C file
    if output_c_file and copy_sections:
        generate_copy_sections_c_file(copy_sections, output_c_file)


def clean_output(output_file):
    """Clean the output by removing the generated LD file."""
    success = True
    
    if os.path.exists(output_file):
        try:
            os.remove(output_file)
            log_success(f"Removed generated LD file: {output_file}")
        except Exception as e:
            log_error(f"Failed to remove output file {output_file}: {str(e)}")
            success = False
    else:
        # 文件不存在时，不报错
        log_info(f"Output LD file does not exist, skipping: {output_file}")
    
    # Also try to remove the corresponding .ld.bfx template file if it exists
    template_file = output_file.replace('.ld', '.ld.bfx') if output_file.endswith('.ld') else None
    if template_file and os.path.exists(template_file):
        try:
            os.remove(template_file)
            log_success(f"Removed template LD file: {template_file}")
        except Exception as e:
            log_error(f"Failed to remove template file {template_file}: {str(e)}")
            success = False
    
    return success


def main():
    parser = argparse.ArgumentParser(description='BufferFlowX LD Script Generator')
    
    # Define arguments
    parser.add_argument('-r', '--regions', help='Path to regions.json file')
    parser.add_argument('-s', '--sections', help='Path to sections.json file')
    parser.add_argument('-t', '--template', help='Path to ld.bfx template file')
    parser.add_argument('-o', '--output', help='Path to output LD script')
    parser.add_argument('-c', '--clean', help='Path to LD script to clean')
    parser.add_argument('--copy-sections-file', help='Path to output file for sections that need copying (for RAM AT FLASH functionality)')
    parser.add_argument('--output-c-file', help='Path to output C file for copying sections before main()')
    
    args = parser.parse_args()
    
    # Check if clean option is specified
    if args.clean:
        clean_output(args.clean)
        return
    
    # Validate required arguments for generation
    missing_args = []
    if not args.regions:
        missing_args.append('-r/--regions')
    if not args.sections:
        missing_args.append('-s/--sections')
    if not args.template:
        missing_args.append('-t/--template')
    if not args.output:
        missing_args.append('-o/--output')
    
    if missing_args:
        parser.error(f"Missing required arguments: {', '.join(missing_args)}")
    
    try:
        generate_ld_script(args.regions, args.sections, args.template, args.output, args.copy_sections_file, args.output_c_file)
        log_success(f"Successfully generated LD script: {args.output}")
    except Exception as e:
        log_error(f"Error generating LD script: {str(e)}")
        sys.exit(1)


if __name__ == '__main__':
    main()