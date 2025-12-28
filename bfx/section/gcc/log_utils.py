#!/usr/bin/env python3
"""
BufferFlowX Logging Utilities
This module provides unified logging functionality for all BufferFlowX tools.
"""

import sys
from enum import Enum


class LogLevel(Enum):
    INFO = "INFO"
    SUCCESS = "OK"
    WARNING = "WARNING"
    ERROR = "ERROR"


def log_message(level: LogLevel, message: str, prefix: str = "BFX", show_time: bool = False):
    """
    Log a message with consistent formatting.
    
    Args:
        level: The log level (INFO, SUCCESS, WARNING, ERROR)
        message: The message to log
        prefix: The prefix to use (default: "BFX")
        show_time: Whether to include timestamp (default: False)
    """
    if level == LogLevel.ERROR:
        # Output errors to stderr
        print(f"[{prefix}] [{level.value}] {message}", file=sys.stderr)
    else:
        # Output everything else to stdout
        print(f"[{prefix}] [{level.value}] {message}")


def log_info(message: str, prefix: str = "BFX"):
    """Log an info message."""
    log_message(LogLevel.INFO, message, prefix)


def log_success(message: str, prefix: str = "BFX"):
    """Log a success message."""
    log_message(LogLevel.SUCCESS, message, prefix)


def log_warning(message: str, prefix: str = "BFX"):
    """Log a warning message."""
    log_message(LogLevel.WARNING, message, prefix)


def log_error(message: str, prefix: str = "BFX"):
    """Log an error message."""
    log_message(LogLevel.ERROR, message, prefix)


def log_step(step: int, description: str, prefix: str = "BFX"):
    """Log a workflow step."""
    log_info(f"Step {step}: {description}", prefix)


def exit_with_error(message: str, prefix: str = "BFX", exit_code: int = 1):
    """Log an error and exit with the specified code."""
    log_error(message, prefix)
    sys.exit(exit_code)