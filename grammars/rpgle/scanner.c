#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum TokenType {
  FIXED_COMMENT,
  FULLY_FREE_MARKER,
  H_FORM, FIXED_KEYWORDS,
  F_FORM, FIXED_FILE_NAME, FIXED_FILE_TYPE, FIXED_FILE_DESIGNATION,
  FIXED_END_OF_FILE, FIXED_FILE_ADDITION, FIXED_FILE_SEQUENCE,
  FIXED_FILE_FORMAT, FIXED_RECORD_LENGTH, FIXED_LIMITS_PROCESSING,
  FIXED_KEY_LENGTH, FIXED_RECORD_ADDRESS_TYPE, FIXED_FILE_ORGANIZATION,
  FIXED_DEVICE, FIXED_RESERVED,
  D_FORM, FIXED_DEFINITION_NAME, FIXED_EXTERNAL_DESCRIPTION,
  FIXED_DATA_STRUCTURE_TYPE, FIXED_DEFINITION_TYPE, FIXED_FROM_POSITION,
  FIXED_TO_POSITION, FIXED_INTERNAL_DATA_TYPE, FIXED_DECIMAL_POSITIONS,
  I_FORM, FIXED_INPUT_FILE_NAME, FIXED_INPUT_RECORD_CONTROL,
  FIXED_INPUT_PREFIX, FIXED_EXTERNAL_FIELD_NAME, FIXED_DATA_ATTRIBUTES,
  FIXED_DATE_TIME_SEPARATOR, FIXED_DATA_FORMAT, FIXED_FIELD_LOCATION,
  FIXED_INPUT_FIELD_NAME, FIXED_INPUT_CONTROL_LEVEL, FIXED_MATCHING_FIELDS,
  FIXED_FIELD_RECORD_RELATION, FIXED_FIELD_INDICATORS, FIXED_INPUT_TO_END,
  C_FORM, FIXED_CONTROL_LEVEL, FIXED_CONDITIONING_INDICATORS,
  FIXED_FACTOR_1, FIXED_OPERATION, FIXED_FACTOR_2, FIXED_RESULT_FIELD,
  FIXED_FIELD_LENGTH, FIXED_RESULTING_INDICATORS, FIXED_CALCULATION_TO_END,
  FIXED_EXTENDED_FACTOR_2,
  O_FORM, FIXED_OUTPUT_FILE_NAME, FIXED_OUTPUT_RECORD_CONTROL,
  FIXED_OUTPUT_INDICATORS, FIXED_OUTPUT_FIELD_NAME, FIXED_EDIT_CODE,
  FIXED_BLANK_AFTER, FIXED_TO_END_POSITION, FIXED_END_POSITION,
  FIXED_CONSTANT_OR_EDIT_WORD, FIXED_OUTPUT_TO_END,
  P_FORM, FIXED_PROCEDURE_NAME, FIXED_TO_PROCEDURE_BEGIN_END,
  FIXED_PROCEDURE_BEGIN_END, FIXED_TO_KEYWORDS,
  FIXED_DIRECTIVE,
  FIXED_SQL_START,
  FIXED_SQL_CONTINUATION,
  FIXED_SQL_END,
  FIXED_IGNORED_TAIL,
  FIXED_NEWLINE,
};

typedef struct {
  bool extended_calculation;
  bool record_line;
  bool fully_free;
} Scanner;

void *tree_sitter_ibmi_rpgle_external_scanner_create(void) {
  return calloc(1, sizeof(Scanner));
}

void tree_sitter_ibmi_rpgle_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_ibmi_rpgle_external_scanner_serialize(void *payload,
                                                           char *buffer) {
  Scanner *scanner = payload;
  buffer[0] = scanner->extended_calculation ? 1 : 0;
  buffer[1] = scanner->record_line ? 1 : 0;
  buffer[2] = scanner->fully_free ? 1 : 0;
  return 3;
}

void tree_sitter_ibmi_rpgle_external_scanner_deserialize(void *payload,
                                                         const char *buffer,
                                                         unsigned length) {
  Scanner *scanner = payload;
  scanner->extended_calculation = length > 0 && buffer[0] != 0;
  scanner->record_line = length > 1 && buffer[1] != 0;
  scanner->fully_free = length > 2 && buffer[2] != 0;
}

static bool at_line_end(const TSLexer *lexer) {
  return lexer->eof(lexer) || lexer->lookahead == '\n' ||
         lexer->lookahead == '\r';
}

static void consume_to_column(TSLexer *lexer, uint32_t end_column) {
  while (!at_line_end(lexer) && lexer->get_column(lexer) < end_column) {
    if (lexer->lookahead == '\t') {
      break;
    }
    lexer->advance(lexer, false);
  }
  lexer->mark_end(lexer);
}

static bool emit_line(TSLexer *lexer, TSSymbol symbol) {
  consume_to_column(lexer, 80);
  lexer->result_symbol = symbol;
  return true;
}

static bool emit_field(TSLexer *lexer, TSSymbol symbol, uint32_t end_column) {
  if (at_line_end(lexer)) return false;
  consume_to_column(lexer, end_column);
  lexer->result_symbol = symbol;
  return true;
}

static bool is_extended_operation(const char *operation) {
  static const char *const operations[] = {
    "CALLP", "DATA-INTO", "DOW", "DOU", "ELSEIF", "EVAL", "EVAL-CORR",
    "EVALR", "FOR", "IF", "ON-ERROR", "ON-EXCP", "ON-EXIT", "RETURN",
    "SORTA", "WHEN", "WHEN-IN", "WHEN-IS", "XML-INTO",
  };
  for (size_t i = 0; i < sizeof(operations) / sizeof(operations[0]); i++) {
    if (strcmp(operation, operations[i]) == 0) return true;
  }
  return false;
}

static bool emit_operation(Scanner *scanner, TSLexer *lexer) {
  char operation[16] = {0};
  size_t length = 0;
  while (!at_line_end(lexer) && lexer->get_column(lexer) < 35) {
    int32_t character = lexer->lookahead;
    if (character == '(') {
      while (!at_line_end(lexer) && lexer->get_column(lexer) < 35)
        lexer->advance(lexer, false);
      break;
    }
    if (character != ' ' && character != '\t' && length + 1 < sizeof(operation)) {
      if (character >= 'a' && character <= 'z') character -= 'a' - 'A';
      operation[length++] = (char)character;
    }
    lexer->advance(lexer, false);
  }
  lexer->mark_end(lexer);
  scanner->extended_calculation = is_extended_operation(operation);
  lexer->result_symbol = FIXED_OPERATION;
  return true;
}

static bool emit_name_and_select_layout(Scanner *scanner, TSLexer *lexer,
                                        TSSymbol symbol) {
  scanner->record_line = false;
  while (!at_line_end(lexer) && lexer->get_column(lexer) < 16) {
    if (lexer->lookahead != ' ' && lexer->lookahead != '\t')
      scanner->record_line = true;
    lexer->advance(lexer, false);
  }
  lexer->mark_end(lexer);
  lexer->result_symbol = symbol;
  return true;
}

static bool is_specification_form(int32_t form) {
  switch (form) {
    case 'H': case 'h': case 'F': case 'f': case 'D': case 'd':
    case 'I': case 'i': case 'C': case 'c': case 'O': case 'o':
    case 'P': case 'p':
      return true;
    default:
      return false;
  }
}

static bool scan_fully_free_marker(Scanner *scanner, TSLexer *lexer) {
  static const char marker[] = "**FREE";
  for (size_t i = 0; i < sizeof(marker) - 1; i++) {
    int32_t character = lexer->lookahead;
    if (character >= 'a' && character <= 'z') character -= 'a' - 'A';
    if (character != marker[i]) return false;
    lexer->advance(lexer, false);
  }
  consume_to_column(lexer, UINT32_MAX);
  scanner->fully_free = true;
  lexer->result_symbol = FULLY_FREE_MARKER;
  return true;
}

static bool emit_c_slash_line(TSLexer *lexer, const bool *valid_symbols) {
  char prefix[16] = {0};
  size_t length = 0;
  while (!at_line_end(lexer) && lexer->get_column(lexer) < 80) {
    int32_t character = lexer->lookahead;
    if (character >= 'a' && character <= 'z') character -= 'a' - 'A';
    if (length + 1 < sizeof(prefix)) prefix[length++] = (char)character;
    lexer->advance(lexer, false);
  }
  lexer->mark_end(lexer);

  TSSymbol symbol = FIXED_DIRECTIVE;
  if (strncmp(prefix, "/EXEC SQL", 9) == 0) symbol = FIXED_SQL_START;
  if (strncmp(prefix, "/END-EXEC", 9) == 0) symbol = FIXED_SQL_END;
  if (!valid_symbols[symbol]) return false;
  lexer->result_symbol = symbol;
  return true;
}

static bool is_alphanumeric(int32_t c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z');
}

bool tree_sitter_ibmi_rpgle_external_scanner_scan(void *payload,
                                                  TSLexer *lexer,
                                                  const bool *valid_symbols) {
  Scanner *scanner = payload;
  uint32_t column = lexer->get_column(lexer);

  if (valid_symbols[FULLY_FREE_MARKER] && column == 0 &&
      lexer->lookahead == '*') {
    return scan_fully_free_marker(scanner, lexer);
  }

  if (scanner->fully_free) return false;

  if (valid_symbols[FIXED_NEWLINE] &&
      (lexer->lookahead == '\n' || lexer->lookahead == '\r')) {
    if (lexer->lookahead == '\r') lexer->advance(lexer, false);
    if (lexer->lookahead == '\n') lexer->advance(lexer, false);
    lexer->mark_end(lexer);
    lexer->result_symbol = FIXED_NEWLINE;
    return true;
  }

  if (valid_symbols[FIXED_IGNORED_TAIL] && column == 80 &&
      !at_line_end(lexer)) {
    consume_to_column(lexer, UINT32_MAX);
    lexer->result_symbol = FIXED_IGNORED_TAIL;
    return true;
  }

  /* Fixed fields are emitted at the exact boundaries in rpgiv-cols.json. */
  if (column == 6 && valid_symbols[FIXED_KEYWORDS])
    return emit_field(lexer, FIXED_KEYWORDS, 80);
  if (column == 6 && valid_symbols[FIXED_FILE_NAME])
    return emit_field(lexer, FIXED_FILE_NAME, 16);
  if (column == 16 && valid_symbols[FIXED_FILE_TYPE])
    return emit_field(lexer, FIXED_FILE_TYPE, 17);
  if (column == 17 && valid_symbols[FIXED_FILE_DESIGNATION])
    return emit_field(lexer, FIXED_FILE_DESIGNATION, 18);
  if (column == 18 && valid_symbols[FIXED_END_OF_FILE])
    return emit_field(lexer, FIXED_END_OF_FILE, 19);
  if (column == 19 && valid_symbols[FIXED_FILE_ADDITION])
    return emit_field(lexer, FIXED_FILE_ADDITION, 20);
  if (column == 20 && valid_symbols[FIXED_FILE_SEQUENCE])
    return emit_field(lexer, FIXED_FILE_SEQUENCE, 21);
  if (column == 21 && valid_symbols[FIXED_FILE_FORMAT])
    return emit_field(lexer, FIXED_FILE_FORMAT, 22);
  if (column == 22 && valid_symbols[FIXED_RECORD_LENGTH])
    return emit_field(lexer, FIXED_RECORD_LENGTH, 27);
  if (column == 27 && valid_symbols[FIXED_LIMITS_PROCESSING])
    return emit_field(lexer, FIXED_LIMITS_PROCESSING, 28);
  if (column == 28 && valid_symbols[FIXED_KEY_LENGTH])
    return emit_field(lexer, FIXED_KEY_LENGTH, 33);
  if (column == 33 && valid_symbols[FIXED_RECORD_ADDRESS_TYPE])
    return emit_field(lexer, FIXED_RECORD_ADDRESS_TYPE, 34);
  if (column == 34 && valid_symbols[FIXED_FILE_ORGANIZATION])
    return emit_field(lexer, FIXED_FILE_ORGANIZATION, 35);
  if (column == 35 && valid_symbols[FIXED_DEVICE])
    return emit_field(lexer, FIXED_DEVICE, 42);
  if (column == 42 && valid_symbols[FIXED_RESERVED])
    return emit_field(lexer, FIXED_RESERVED, 43);
  if (column == 43 && valid_symbols[FIXED_KEYWORDS])
    return emit_field(lexer, FIXED_KEYWORDS, 80);

  if (column == 6 && valid_symbols[FIXED_DEFINITION_NAME])
    return emit_field(lexer, FIXED_DEFINITION_NAME, 21);
  if (column == 21 && valid_symbols[FIXED_EXTERNAL_DESCRIPTION])
    return emit_field(lexer, FIXED_EXTERNAL_DESCRIPTION, 22);
  if (column == 22 && valid_symbols[FIXED_DATA_STRUCTURE_TYPE])
    return emit_field(lexer, FIXED_DATA_STRUCTURE_TYPE, 23);
  if (column == 23 && valid_symbols[FIXED_DEFINITION_TYPE])
    return emit_field(lexer, FIXED_DEFINITION_TYPE, 25);
  if (column == 25 && valid_symbols[FIXED_FROM_POSITION])
    return emit_field(lexer, FIXED_FROM_POSITION, 32);
  if (column == 32 && valid_symbols[FIXED_TO_POSITION])
    return emit_field(lexer, FIXED_TO_POSITION, 39);
  if (column == 39 && valid_symbols[FIXED_INTERNAL_DATA_TYPE])
    return emit_field(lexer, FIXED_INTERNAL_DATA_TYPE, 40);
  if (column == 40 && valid_symbols[FIXED_DECIMAL_POSITIONS])
    return emit_field(lexer, FIXED_DECIMAL_POSITIONS, 42);

  if (column == 6 && valid_symbols[FIXED_PROCEDURE_NAME])
    return emit_field(lexer, FIXED_PROCEDURE_NAME, 21);
  if (column == 21 && valid_symbols[FIXED_TO_PROCEDURE_BEGIN_END])
    return emit_field(lexer, FIXED_TO_PROCEDURE_BEGIN_END, 23);
  if (column == 23 && valid_symbols[FIXED_PROCEDURE_BEGIN_END])
    return emit_field(lexer, FIXED_PROCEDURE_BEGIN_END, 24);
  if (column == 24 && valid_symbols[FIXED_TO_KEYWORDS])
    return emit_field(lexer, FIXED_TO_KEYWORDS, 43);

  if (column == 6 && valid_symbols[FIXED_CONTROL_LEVEL])
    return emit_field(lexer, FIXED_CONTROL_LEVEL, 8);
  if (column == 8 && valid_symbols[FIXED_CONDITIONING_INDICATORS])
    return emit_field(lexer, FIXED_CONDITIONING_INDICATORS, 11);
  if (column == 11 && valid_symbols[FIXED_FACTOR_1])
    return emit_field(lexer, FIXED_FACTOR_1, 25);
  if (column == 25 && valid_symbols[FIXED_OPERATION])
    return emit_operation(scanner, lexer);
  if (column == 35 && scanner->extended_calculation &&
      valid_symbols[FIXED_EXTENDED_FACTOR_2])
    return emit_field(lexer, FIXED_EXTENDED_FACTOR_2, 80);
  if (column == 35 && !scanner->extended_calculation &&
      valid_symbols[FIXED_FACTOR_2])
    return emit_field(lexer, FIXED_FACTOR_2, 49);
  if (column == 49 && valid_symbols[FIXED_RESULT_FIELD])
    return emit_field(lexer, FIXED_RESULT_FIELD, 63);
  if (column == 63 && valid_symbols[FIXED_FIELD_LENGTH])
    return emit_field(lexer, FIXED_FIELD_LENGTH, 68);
  if (column == 68 && valid_symbols[FIXED_DECIMAL_POSITIONS])
    return emit_field(lexer, FIXED_DECIMAL_POSITIONS, 70);
  if (column == 70 && valid_symbols[FIXED_RESULTING_INDICATORS])
    return emit_field(lexer, FIXED_RESULTING_INDICATORS, 76);
  if (column == 76 && valid_symbols[FIXED_CALCULATION_TO_END])
    return emit_field(lexer, FIXED_CALCULATION_TO_END, 80);

  if (column == 6 && valid_symbols[FIXED_INPUT_FILE_NAME])
    return emit_name_and_select_layout(scanner, lexer, FIXED_INPUT_FILE_NAME);
  if (column == 16 && scanner->record_line &&
      valid_symbols[FIXED_INPUT_RECORD_CONTROL])
    return emit_field(lexer, FIXED_INPUT_RECORD_CONTROL, 46);
  if (column == 16 && !scanner->record_line && valid_symbols[FIXED_INPUT_PREFIX])
    return emit_field(lexer, FIXED_INPUT_PREFIX, 20);
  if (column == 20 && valid_symbols[FIXED_EXTERNAL_FIELD_NAME])
    return emit_field(lexer, FIXED_EXTERNAL_FIELD_NAME, 30);
  if (column == 30 && valid_symbols[FIXED_DATA_ATTRIBUTES])
    return emit_field(lexer, FIXED_DATA_ATTRIBUTES, 34);
  if (column == 34 && valid_symbols[FIXED_DATE_TIME_SEPARATOR])
    return emit_field(lexer, FIXED_DATE_TIME_SEPARATOR, 35);
  if (column == 35 && valid_symbols[FIXED_DATA_FORMAT])
    return emit_field(lexer, FIXED_DATA_FORMAT, 36);
  if (column == 36 && valid_symbols[FIXED_FIELD_LOCATION])
    return emit_field(lexer, FIXED_FIELD_LOCATION, 46);
  if (column == 46 && valid_symbols[FIXED_DECIMAL_POSITIONS])
    return emit_field(lexer, FIXED_DECIMAL_POSITIONS, 48);
  if (column == 48 && valid_symbols[FIXED_INPUT_FIELD_NAME])
    return emit_field(lexer, FIXED_INPUT_FIELD_NAME, 62);
  if (column == 62 && valid_symbols[FIXED_INPUT_CONTROL_LEVEL])
    return emit_field(lexer, FIXED_INPUT_CONTROL_LEVEL, 64);
  if (column == 64 && valid_symbols[FIXED_MATCHING_FIELDS])
    return emit_field(lexer, FIXED_MATCHING_FIELDS, 66);
  if (column == 66 && valid_symbols[FIXED_FIELD_RECORD_RELATION])
    return emit_field(lexer, FIXED_FIELD_RECORD_RELATION, 68);
  if (column == 68 && valid_symbols[FIXED_FIELD_INDICATORS])
    return emit_field(lexer, FIXED_FIELD_INDICATORS, 74);
  if ((column == 46 || column == 74) && valid_symbols[FIXED_INPUT_TO_END])
    return emit_field(lexer, FIXED_INPUT_TO_END, 80);

  if (column == 6 && valid_symbols[FIXED_OUTPUT_FILE_NAME])
    return emit_name_and_select_layout(scanner, lexer, FIXED_OUTPUT_FILE_NAME);
  if (column == 16 && scanner->record_line &&
      valid_symbols[FIXED_OUTPUT_RECORD_CONTROL])
    return emit_field(lexer, FIXED_OUTPUT_RECORD_CONTROL, 51);
  if (column == 16 && !scanner->record_line &&
      valid_symbols[FIXED_OUTPUT_INDICATORS])
    return emit_field(lexer, FIXED_OUTPUT_INDICATORS, 29);
  if (column == 29 && valid_symbols[FIXED_OUTPUT_FIELD_NAME])
    return emit_field(lexer, FIXED_OUTPUT_FIELD_NAME, 43);
  if (column == 43 && valid_symbols[FIXED_EDIT_CODE])
    return emit_field(lexer, FIXED_EDIT_CODE, 44);
  if (column == 44 && valid_symbols[FIXED_BLANK_AFTER])
    return emit_field(lexer, FIXED_BLANK_AFTER, 45);
  if (column == 45 && valid_symbols[FIXED_TO_END_POSITION])
    return emit_field(lexer, FIXED_TO_END_POSITION, 46);
  if (column == 46 && valid_symbols[FIXED_END_POSITION])
    return emit_field(lexer, FIXED_END_POSITION, 51);
  if (column == 51 && valid_symbols[FIXED_DATA_FORMAT])
    return emit_field(lexer, FIXED_DATA_FORMAT, 52);
  if (column == 52 && valid_symbols[FIXED_CONSTANT_OR_EDIT_WORD])
    return emit_field(lexer, FIXED_CONSTANT_OR_EDIT_WORD, 80);
  if (column == 51 && valid_symbols[FIXED_OUTPUT_TO_END])
    return emit_field(lexer, FIXED_OUTPUT_TO_END, 80);

  if ((column != 0 && column != 5) || at_line_end(lexer)) return false;

  /* Positions 1-5 are the sequence area; the form type must be in 6. */
  bool sequence_blank = true; /* positions 1-5 hold only blanks or digits */
  if (column == 0) {
    while (!at_line_end(lexer) && lexer->get_column(lexer) < 5 &&
           lexer->lookahead != '\t') {
      if (lexer->lookahead != ' ' &&
          (lexer->lookahead < '0' || lexer->lookahead > '9')) {
        sequence_blank = false;
      }
      lexer->advance(lexer, false);
    }
    lexer->mark_end(lexer);
  }
  if (lexer->get_column(lexer) != 5 || at_line_end(lexer)) return false;

  int32_t form = lexer->lookahead;
  lexer->advance(lexer, false);

  /* An asterisk in position 7 makes a comment line whatever stands in position
   * 6: blank, a specification letter, or a decoration such as `/*` or `**`. A
   * letter or digit is left alone so that free-form `amount*=rate;` is not hit. */
  if (!is_alphanumeric(form) && lexer->get_column(lexer) == 6 &&
      lexer->lookahead == '*' && valid_symbols[FIXED_COMMENT]) {
    return emit_line(lexer, FIXED_COMMENT);
  }

  if (form == ' ' && lexer->get_column(lexer) == 6 &&
      lexer->lookahead == '/' && valid_symbols[FIXED_DIRECTIVE]) {
    return emit_line(lexer, FIXED_DIRECTIVE);
  }

  if ((form == 'C' || form == 'c') && lexer->get_column(lexer) == 6 &&
      lexer->lookahead == '/') {
    return emit_c_slash_line(lexer, valid_symbols);
  }

  if ((form == 'C' || form == 'c') && lexer->get_column(lexer) == 6 &&
      lexer->lookahead == '+' && valid_symbols[FIXED_SQL_CONTINUATION]) {
    return emit_line(lexer, FIXED_SQL_CONTINUATION);
  }

  /* Other directives may be attached to any specification form. */
  if (is_specification_form(form) && lexer->get_column(lexer) == 6 &&
      lexer->lookahead == '/' &&
      valid_symbols[FIXED_DIRECTIVE]) {
    return emit_line(lexer, FIXED_DIRECTIVE);
  }

  if (is_specification_form(form) && lexer->get_column(lexer) == 6 &&
      lexer->lookahead == '*' &&
      valid_symbols[FIXED_COMMENT]) {
    return emit_line(lexer, FIXED_COMMENT);
  }

  TSSymbol symbol;
  switch (form) {
    case 'H': case 'h': symbol = H_FORM; break;
    case 'F': case 'f': symbol = F_FORM; break;
    case 'D': case 'd': symbol = D_FORM; break;
    case 'I': case 'i': symbol = I_FORM; scanner->record_line = false; break;
    case 'C': case 'c': symbol = C_FORM; scanner->extended_calculation = false; break;
    case 'O': case 'o': symbol = O_FORM; scanner->record_line = false; break;
    case 'P': case 'p': symbol = P_FORM; break;
    default: return false;
  }

  if (!valid_symbols[symbol]) return false;
  /* `Agosto` is a data line, not a blank O specification. */
  if (at_line_end(lexer) && !sequence_blank) return false;
  /* The form letter is already consumed. Do not go through emit_field: it
   * refuses to emit at the end of a line, and a line holding nothing but the
   * form letter is a valid blank specification. */
  lexer->mark_end(lexer);
  lexer->result_symbol = symbol;
  return true;
}
