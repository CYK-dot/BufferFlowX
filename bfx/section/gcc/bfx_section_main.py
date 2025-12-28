#!/usr/bin/env python3
"""
BufferFlowX Section Workflow Manager
This script manages the complete workflow for section processing:
1. Collects bfx.yml files from components
2. Converts bfx.yml to JSON format
3. Extracts region information from linker script
4. Generates final LD script
"""

import argparse
import os
import subprocess
import sys
from log_utils import log_info, log_error, log_success, log_step, exit_with_error


def run_command(cmd, description):
    """Execute a command and handle errors."""
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"{e.stderr}")
        return False
    except FileNotFoundError:
        print(f"Command not found: {cmd[0]}")
        return False


def generate_workflow(args):
    """Execute the complete generation workflow."""

    get_section_cmd = [
        sys.executable, 
        os.path.join(os.path.dirname(__file__), 'bfx_get_section.py')
    ]
    
    if args.config:
        get_section_cmd.extend(['--config', args.config])
    
    if args.project_root:
        get_section_cmd.extend(['-p', args.project_root])
    
    # For bfx_get_section.py, we need to create a temporary JSON file path
    temp_json_path = os.path.join(args.output_dir, 'sections.json') if args.output_dir else 'sections.json'
    get_section_cmd.extend(['-t', temp_json_path])
    
    # Add --clean if specified
    if args.clean:
        get_section_cmd.extend(['-c', temp_json_path])
    
    if not run_command(get_section_cmd, "Collecting and converting bfx.yml files to JSON"):
        return False
    
    # Prepare command arguments for bfx_get_region.py
    get_region_cmd = [
        sys.executable,
        os.path.join(os.path.dirname(__file__), 'bfx_get_region.py')
    ]
    
    if args.config:
        get_region_cmd.extend(['--config', args.config])
    
    if args.ld_template:
        get_region_cmd.extend(['-f', args.ld_template])
    
    if args.output_dir:
        get_region_cmd.extend(['-t', args.output_dir])
    
    # Create output file path for regions.json
    regions_json_path = os.path.join(args.output_dir, 'regions.json') if args.output_dir else 'regions.json'
    get_region_cmd.extend(['-o', regions_json_path])
    
    # Add --clean if specified
    if args.clean:
        get_region_cmd.extend(['-c', args.output_dir, regions_json_path])
    
    if not run_command(get_region_cmd, "Extracting region information from linker script"):
        return False
    
    # Prepare command arguments for bfx_generate_ld.py
    generate_ld_cmd = [
        sys.executable,
        os.path.join(os.path.dirname(__file__), 'bfx_generate_ld.py')
    ]
    
    if args.config:
        generate_ld_cmd.extend(['--config', args.config])
    
    # Create paths for required input files
    regions_json_path = os.path.join(args.output_dir, 'regions.json') if args.output_dir else 'regions.json'
    sections_json_path = os.path.join(args.output_dir, 'sections.json') if args.output_dir else 'sections.json'
    
    generate_ld_cmd.extend(['-r', regions_json_path])
    generate_ld_cmd.extend(['-s', sections_json_path])
    generate_ld_cmd.extend(['-t', args.ld_template])
    generate_ld_cmd.extend(['-o', args.output_ld])
    
    # Add optional parameters for RAM AT FLASH functionality
    if args.copy_sections_file:
        generate_ld_cmd.extend(['--copy-sections-file', args.copy_sections_file])
    
    if args.output_c_file:
        generate_ld_cmd.extend(['--output-c-file', args.output_c_file])
    
    # Add --clean if specified
    if args.clean:
        generate_ld_cmd.extend(['-c', args.output_ld])
    
    if not run_command(generate_ld_cmd, "Generating final LD script"):
        return False
    
    log_success("Generation workflow completed successfully!")
    return True


def clean_workflow(args):
    """Clean the generated files."""
    
    # Define paths - use the same directory structure as generation
    output_dir = args.output_dir if args.output_dir else os.path.join(args.project_root, "output")
    temp_dir = output_dir  # The .ld.bfx files are generated in the output_dir, not in a separate temp subdirectory
    regions_json_path = os.path.join(output_dir, "regions.json")
    sections_json_path = os.path.join(output_dir, "sections.json")
    
    # Clean up sections.json
    clean_section_cmd = [
        sys.executable,
        os.path.join(os.path.dirname(__file__), "bfx_get_section.py"),
        "-c", sections_json_path
    ]
    result = subprocess.run(clean_section_cmd, capture_output=True, text=True)
    
    # Clean up regions.json and .ld.bfx files
    clean_region_cmd = [
        sys.executable,
        os.path.join(os.path.dirname(__file__), "bfx_get_region.py"),
        "-c", temp_dir, regions_json_path
    ]
    result = subprocess.run(clean_region_cmd, capture_output=True, text=True)
    # 不检查返回码，因为文件不存在时也会返回非零码，但这是正常的

    # Clean up generated LD script
    clean_generate_cmd = [
        sys.executable,
        os.path.join(os.path.dirname(__file__), "bfx_generate_ld.py"),
        "-c", args.output_ld
    ]
    result = subprocess.run(clean_generate_cmd, capture_output=True, text=True)
    # 不检查返回码，因为文件不存在时也会返回非零码，但这是正常的
    if result.returncode != 0:
        log_info(f"Generated LD cleaning result: {result.stderr.strip() or 'Completed'}")
    
    # Clean up generated C file if specified
    if args.output_c_file and os.path.exists(args.output_c_file):
        try:
            os.remove(args.output_c_file)
            log_success(f"Removed generated C file: {args.output_c_file}")
        except Exception as e:
            log_error(f"Failed to remove C file {args.output_c_file}: {str(e)}")
    
    log_success("Clean workflow completed successfully!")
    return True


def main():
    parser = argparse.ArgumentParser(description='BufferFlowX Section Workflow Manager')
    
    # Configuration file (optional, for backward compatibility)
    parser.add_argument('--config', help='Path to JSON configuration file')
    
    # Individual arguments that can override config file
    parser.add_argument('--ld-template', help='Path to LD template file (overrides config)')
    parser.add_argument('--project-root', help='Project root directory (overrides config)')
    parser.add_argument('--output-dir', help='Output directory for JSON files (overrides config)')
    parser.add_argument('--output-ld', help='Output LD script file path (overrides config)')
    
    # New arguments for RAM AT FLASH functionality
    parser.add_argument('--copy-sections-file', help='Path to output file for sections that need copying (for RAM AT FLASH functionality)')
    parser.add_argument('--output-c-file', help='Path to output C file for copying sections before main()')
    
    # Operation mode
    parser.add_argument('--clean', action='store_true', help='Clean generated files instead of generating')
    
    args = parser.parse_args()
    
    # Validate configuration
    if args.config and not os.path.exists(args.config):
        exit_with_error(f"Configuration file does not exist: {args.config}")
    
    # Execute appropriate workflow
    if args.clean:
        success = clean_workflow(args)
    else:
        success = generate_workflow(args)

if __name__ == '__main__':
    main()