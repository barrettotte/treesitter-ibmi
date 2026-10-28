# Changelog

This project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

- RPGLE: accept valid free-form constructs that gave `ERROR` or `MISSING` nodes (see `test/corpus/free_form_constructs.txt`).

## [0.1.0]

- Initial Tree-sitter parsers for ILE RPG, RPG/400, ILE COBOL, CL, CMD, MI,
  binder language, and UIM panel groups.
- Dedicated DDS parsers for physical files, logical files, ICF files, display
  files, and printer files.
- Dedicated Db2 for i SQL parsers for RPGLE, RPG/400, and COBOL host-language
  forms, including embedded-SQL injection support.
- Column-aware parsing for reference-format and fixed-format IBM i source,
  including source-type-specific column constraints.
- Highlight, injection, fold, indent, and text-object queries where supported
  by each language.
