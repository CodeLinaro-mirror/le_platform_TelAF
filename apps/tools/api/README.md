# README - API ID Fixer Script

# API Versioning
API versioning is critical for maintaining backward compatibility as interfaces evolve. The script ensures:
- Each API file has `API_VERSION` and `SERVING_VERSION` declarations
- Every `FUNCTION` and `EVENT` has proper ID assignments
- IDs are unique within each file
This versioning allows clients and services to negotiate compatible API versions during binding.

# Functionality
- Identifies `EVENT` and `FUNCTION` blocks in `.api` files.
- Ensures each `EVENT` has two unique IDs and each `FUNCTION` has one unique ID.
- Inserts missing ID assignments or replaces invalid/duplicate IDs.
- Optionally renumbers all IDs from a specified starting value.
- Preserves original formatting and comments.
- Supports dry-run mode to preview changes.
- Creates backup files before modifying originals (unless disabled).

# Usage
Run the script from the command line:

    python script.py [options] <path>

Where `<path>` is a file or directory containing `.api` files.

# Options
--no-backup       Do not create `.bak` backup files before modifying originals.
--dry-run         Preview changes without writing to files.
--reset-ids       Force renumbering of all IDs from `--start-id`, ignoring existing IDs.
--start-id N      Specify starting ID when using `--reset-ids` (default is 0).

# Example
To scan a directory and fix IDs without modifying files:

    python script.py --dry-run ./api_files

To reset all IDs starting from 100 and create backups:

    python script.py --reset-ids --start-id 100 ./api_files

# Notes
- Only `.api` files are processed.
- IDs are unique only within each file.
- The script ignores comments and string literals when searching for ID blocks.
