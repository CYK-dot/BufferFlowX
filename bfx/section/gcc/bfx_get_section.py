#!/usr/bin/env python3
"""
bfx_collect_section.py - Collect bfx.yml files from project and merge them into a JSON file

Usage:
    bfx_collect_section.py -p <project_root> -t <temp_json_file>
    bfx_collect_section.py -c <temp_json_file>

Options:
    -p <project_root>    Project root directory to search for bfx.yml files
    -t <temp_json_file>  Temporary JSON file output directory and name
    -c <temp_json_file>  Clean (remove) the temporary JSON file
"""

import os
import sys
import yaml
import json
import argparse
from pathlib import Path
from log_utils import log_info, log_error, log_success, exit_with_error


def find_bfx_yml_files(project_root):
    """
    Recursively search for bfx.yml files in the project root directory
    """
    bfx_files = []
    for root, dirs, files in os.walk(project_root):
        for file in files:
            if file.lower().endswith('bfx.yml'):
                bfx_files.append(os.path.join(root, file))
    return bfx_files


def validate_bfx_yml_content(content, file_path):
    """
    Validate the content of a bfx.yml file
    """
    errors = []
    
    # Check if 'name' field exists
    if 'name' not in content:
        errors.append(f"Missing required field 'name'")
    
    # Check if 'section' field exists and is a list
    if 'section' not in content:
        errors.append(f"Missing required field 'section'")
    elif not isinstance(content['section'], list):
        errors.append(f"Field 'section' must be a list")
    
    # If section exists and is a list, validate each section
    if 'section' in content and isinstance(content['section'], list):
        for i, section in enumerate(content['section']):
            if not isinstance(section, dict):
                errors.append(f"Section at index {i} must be a dictionary")
                continue
            
            # Check required fields in each section
            required_fields = ['name', 'region']
            for field in required_fields:
                if field not in section:
                    errors.append(f"Section at index {i} missing required field '{field}'")
            
            # Check for invalid fields (in a real implementation you might want to define valid fields)
            valid_fields = ['name', 'region', 'align', 'address', 'size', 'load_region']
            for field in section:
                if field not in valid_fields:
                    errors.append(f"Section at index {i} has invalid field '{field}'")
    
    return errors


def load_bfx_yml_content(file_path):
    """
    Load and parse a bfx.yml file
    """
    with open(file_path, 'r', encoding='utf-8') as f:
        content = yaml.safe_load(f)
        return content


def collect_sections(project_root):
    """
    Collect all bfx.yml files and merge their content with validation
    """
    bfx_files = find_bfx_yml_files(project_root)
    collected_data = {
        "project_root": project_root,
        "bfx_files": [],
        "sections": {}
    }
    
    for file_path in bfx_files:
        try:
            content = load_bfx_yml_content(file_path)
            if content:
                # Validate the content
                validation_errors = validate_bfx_yml_content(content, file_path)
                if validation_errors:
                    for error in validation_errors:
                        log_error(f"File {file_path}: {error}")
                    continue  # Skip this file if validation fails
                
                # Add file path info to the content
                file_info = {
                    "file_path": file_path,
                    "content": content
                }
                collected_data["bfx_files"].append(file_info)
                
                # Extract component name
                component_name = content.get('name', '')
                
                # Initialize component in sections if not exists
                if component_name not in collected_data["sections"]:
                    collected_data["sections"][component_name] = []
                
                # Extract section information and check for duplicates within the same component
                if 'section' in content:
                    for section in content['section']:
                        section_name = section.get('name', '')
                        
                        # Check if this section name already exists in the same component
                        existing_section_names = [s['section_name'] for s in collected_data["sections"][component_name]]
                        if section_name in existing_section_names:
                            log_error(f"Duplicate section name '{section_name}' in component '{component_name}' from file {file_path}")
                            return None  # Return failure when duplicate section found
                        
                        section_info = {
                            "source_file": file_path,
                            "component_name": component_name,
                            "section_name": section_name,
                            "region": section.get('region', ''),
                            "load_region": section.get('load_region', ''),  # New field for RAM AT FLASH
                            "align": int(section.get('align', 0)) if section.get('align', '') != '' else 0,
                            "address": int(section.get('address', 0)) if section.get('address', '') != '' else 0,
                            "size": int(section.get('size', 0)) if section.get('size', '') != '' else 0
                        }
                        collected_data["sections"][component_name].append(section_info)
        except yaml.YAMLError as e:
            log_error(f"YAML parsing error in file {file_path}: {str(e)}")
            continue
        except Exception as e:
            log_error(f"Error processing file {file_path}: {str(e)}")
            # 继续处理其他文件，而不是返回None
            continue
    
    return collected_data


def save_to_json(data, json_file_path):
    """
    Save collected data to a JSON file
    """
    try:
        os.makedirs(os.path.dirname(json_file_path), exist_ok=True)
        with open(json_file_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        return True
    except Exception as e:
        log_error(f"Failed to save JSON file {json_file_path}: {str(e)}")
        return False


def clean_json_file(json_file_path):
    """
    Remove the JSON file if it exists
    """
    if os.path.exists(json_file_path):
        try:
            os.remove(json_file_path)
            log_success(f"Removed {json_file_path}")
            return True
        except Exception as e:
            log_error(f"Failed to remove {json_file_path}: {str(e)}")
            return False
    else:
        # 文件不存在时，不报错，直接返回True
        log_info(f"JSON file does not exist, skipping: {json_file_path}")
        return True


def main():
    parser = argparse.ArgumentParser(description='Collect bfx.yml files and merge them into a JSON file')
    parser.add_argument('-p', '--project', help='Project root directory to search for bfx.yml files')
    parser.add_argument('-t', '--temp', help='Temporary JSON file output directory and name')
    parser.add_argument('-c', '--clean', help='Clean (remove) the temporary JSON file')
    
    args = parser.parse_args()
    
    # Check if we're in clean mode
    if args.clean:
        success = clean_json_file(args.clean)
        return 0 if success else 1
    
    # Validate required arguments for collection mode
    if not args.project or not args.temp:
        log_error("Both -p (project root) and -t (temp JSON file) are required for collection mode.")
        parser.print_help()
        return 1
    
    # Validate that project directory exists
    if not os.path.isdir(args.project):
        log_error(f"Project directory {args.project} does not exist.")
        return 1
    
    # Collect sections from bfx.yml files
    collected_data = collect_sections(args.project)
    
    # Check if collection was successful
    if collected_data is None:
        return 1
    
    # Save to JSON file
    save_success = save_to_json(collected_data, args.temp)
    
    if not save_success:
        return 1
    
    log_success(f"Successfully collected {len(collected_data['bfx_files'])} bfx.yml files")
    total_sections = sum(len(sections) for sections in collected_data['sections'].values())
    log_success(f"Found {len(collected_data['sections'])} components with {total_sections} sections total")
    log_success(f"Output saved to {args.temp}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())