const BUILTINS = require("../../data/rpgle-builtins.json");

module.exports = grammar({
  name: "ibmi_rpgle",

  externals: ($) => [
    $.fixed_comment,
    $.fully_free_marker,
    $._h_form, $.fixed_keywords,
    $._f_form, $.fixed_file_name, $.fixed_file_type,
    $.fixed_file_designation, $.fixed_end_of_file, $.fixed_file_addition,
    $.fixed_file_sequence, $.fixed_file_format, $.fixed_record_length,
    $.fixed_limits_processing, $.fixed_key_length,
    $.fixed_record_address_type, $.fixed_file_organization,
    $.fixed_device, $._fixed_reserved,
    $._d_form, $.fixed_definition_name, $.fixed_external_description,
    $.fixed_data_structure_type, $.fixed_definition_type,
    $.fixed_from_position, $.fixed_to_position, $.fixed_internal_data_type,
    $.fixed_decimal_positions,
    $._i_form, $.fixed_input_file_name, $.fixed_input_record_control,
    $._fixed_input_prefix, $.fixed_external_field_name,
    $.fixed_data_attributes, $.fixed_date_time_separator,
    $.fixed_data_format, $.fixed_field_location,
    $.fixed_input_field_name, $.fixed_input_control_level,
    $.fixed_matching_fields, $.fixed_field_record_relation,
    $.fixed_field_indicators, $._fixed_input_to_end,
    $._c_form, $.fixed_control_level, $.fixed_conditioning_indicators,
    $.fixed_factor_1, $.fixed_operation, $.fixed_factor_2,
    $.fixed_result_field, $.fixed_field_length,
    $.fixed_resulting_indicators, $._fixed_calculation_to_end,
    $.fixed_extended_factor_2,
    $._o_form, $.fixed_output_file_name, $.fixed_output_record_control,
    $.fixed_output_indicators, $.fixed_output_field_name,
    $.fixed_edit_code, $.fixed_blank_after, $._fixed_to_end_position,
    $.fixed_end_position,
    $.fixed_constant_or_edit_word, $._fixed_output_to_end,
    $._p_form, $.fixed_procedure_name, $._fixed_to_procedure_begin_end,
    $.fixed_procedure_begin_end, $._fixed_to_keywords,
    $.fixed_directive,
    $.fixed_sql_start,
    $.fixed_sql_continuation,
    $.fixed_sql_end,
    $._fixed_ignored_tail,
    $._fixed_newline,
  ],

  extras: ($) => [/\s/, $.comment],

  word: ($) => $.identifier,

  conflicts: ($) => [
    [$.opcode, $.expression],
    [$.source_file, $._body_item],
  ],

  rules: {
    // Compile-time data runs to the end of the source: nothing but more data (or the
    // next `**` header) may follow, so a data line such as `Category` is never read as a
    // specification whose form letter happens to stand in position 6.
    source_file: ($) =>
      seq(
        repeat(choice(
          $.fully_free_marker,
          $._fixed_newline,
          $.fixed_sql_statement,
          $._fixed_line,
          $.legacy_free_end,
          $._top_level_item,
        )),
        repeat($.compile_time_data),
      ),

    // The free-form lexer has already consumed columns 1-5 when this delimiter
    // follows a free statement; its opening delimiter remains scanner-owned.
    legacy_free_end: () =>
      token(prec(4, /[Cc]\/END-FREE[^\r\n]*\r?\n/i)),

    _fixed_line: ($) =>
      seq(
        optional($._fixed_sequence),
        choice(
          $.fixed_comment,
          $.fixed_control_specification,
          $.fixed_file_specification,
          $.fixed_definition_specification,
          $.fixed_input_specification,
          $.fixed_calculation_specification,
          $.fixed_output_specification,
          $.fixed_procedure_specification,
          $.fixed_directive,
        ),
        optional($._fixed_ignored_tail),
      ),

    _fixed_sequence: () => token(prec(1, /[0-9]{5}/)),

    fixed_sql_statement: ($) =>
      seq(
        $.fixed_sql_start,
        optional($._fixed_ignored_tail),
        $._fixed_newline,
        repeat(choice(
          seq(
            $.fixed_sql_continuation,
            optional($._fixed_ignored_tail),
            $._fixed_newline,
          ),
          seq(
            $.fixed_comment,
            optional($._fixed_ignored_tail),
            $._fixed_newline,
          ),
        )),
        $.fixed_sql_end,
        optional($._fixed_ignored_tail),
      ),

    fixed_control_specification: ($) =>
      prec(10, seq($._h_form, optional($.fixed_keywords))),

    fixed_file_specification: ($) =>
      prec(10, seq($._f_form, optional(optionalNested($, [
        ["name", $.fixed_file_name],
        ["file_type", $.fixed_file_type],
        ["designation", $.fixed_file_designation],
        ["end_of_file", $.fixed_end_of_file],
        ["addition", $.fixed_file_addition],
        ["sequence", $.fixed_file_sequence],
        ["format", $.fixed_file_format],
        ["record_length", $.fixed_record_length],
        ["limits_processing", $.fixed_limits_processing],
        ["key_length", $.fixed_key_length],
        ["record_address_type", $.fixed_record_address_type],
        ["organization", $.fixed_file_organization],
        ["device", $.fixed_device],
        [null, $._fixed_reserved],
        ["keywords", $.fixed_keywords],
      ])))),

    fixed_definition_specification: ($) =>
      prec(10, seq($._d_form, optional(optionalNested($, [
        ["name", $.fixed_definition_name],
        ["external_description", $.fixed_external_description],
        ["data_structure_type", $.fixed_data_structure_type],
        ["definition_type", $.fixed_definition_type],
        ["from_position", $.fixed_from_position],
        ["to_position", $.fixed_to_position],
        ["data_type", $.fixed_internal_data_type],
        ["decimal_positions", $.fixed_decimal_positions],
        [null, $._fixed_reserved],
        ["keywords", $.fixed_keywords],
      ])))),

    fixed_procedure_specification: ($) =>
      prec(10, seq($._p_form, optional(optionalNested($, [
        ["name", $.fixed_procedure_name],
        [null, $._fixed_to_procedure_begin_end],
        ["kind", $.fixed_procedure_begin_end],
        [null, $._fixed_to_keywords],
        ["keywords", $.fixed_keywords],
      ])))),

    fixed_calculation_specification: ($) =>
      prec(10, seq($._c_form, optional(optionalNested($, [
        ["control_level", $.fixed_control_level],
        ["conditioning", $.fixed_conditioning_indicators],
        ["factor_1", $.fixed_factor_1],
        ["operation", $.fixed_operation],
        [null, choice(
          field("extended_factor_2", $.fixed_extended_factor_2),
          seq(
            field("factor_2", $.fixed_factor_2),
            optional(optionalNested($, [
              ["result", $.fixed_result_field],
              ["length", $.fixed_field_length],
              ["decimal_positions", $.fixed_decimal_positions],
              ["resulting_indicators", $.fixed_resulting_indicators],
              [null, $._fixed_calculation_to_end],
            ])),
          ),
        )],
      ])))),

    fixed_input_specification: ($) =>
      prec(10, seq(
        $._i_form,
        optional(seq(
          field("file_name", $.fixed_input_file_name),
          optional(choice(
            seq(
              field("record_control", $.fixed_input_record_control),
              optional($._fixed_input_to_end),
            ),
            seq(
              $._fixed_input_prefix,
              field("external_field_name", $.fixed_external_field_name),
              optional(optionalNested($, [
                ["data_attributes", $.fixed_data_attributes],
                ["separator", $.fixed_date_time_separator],
                ["format", $.fixed_data_format],
                ["location", $.fixed_field_location],
                ["decimal_positions", $.fixed_decimal_positions],
                ["field_name", $.fixed_input_field_name],
                ["control_level", $.fixed_input_control_level],
                ["matching_fields", $.fixed_matching_fields],
                ["record_relation", $.fixed_field_record_relation],
                ["indicators", $.fixed_field_indicators],
                [null, $._fixed_input_to_end],
              ])),
            ),
          )),
        )),
      )),

    fixed_output_specification: ($) =>
      prec(10, seq(
        $._o_form,
        optional(seq(
          field("file_name", $.fixed_output_file_name),
          optional(choice(
            seq(
              field("record_control", $.fixed_output_record_control),
              optional($._fixed_output_to_end),
            ),
            seq(
              field("indicators", $.fixed_output_indicators),
              field("field_name", $.fixed_output_field_name),
              optional(optionalNested($, [
                ["edit_code", $.fixed_edit_code],
                ["blank_after", $.fixed_blank_after],
                [null, $._fixed_to_end_position],
                ["end_position", $.fixed_end_position],
                ["format", $.fixed_data_format],
                ["constant_or_edit_word", $.fixed_constant_or_edit_word],
              ])),
            ),
          )),
        )),
      )),

    _top_level_item: ($) =>
      choice(
        $.procedure,
        $._body_item,
      ),

    compiler_directive: () =>
      token(prec(3, seq("/", /[A-Za-z]+/, /[^\r\n]*/, optional(/\r?\n/)))),

    compile_time_data: ($) =>
      seq(
        field("header", $.compile_time_data_header),
        repeat(field("value", $.compile_time_data_value)),
      ),

    // `**CTDATA name`, or `**` / `** name` alone: what follows, up to the next header, is data
    compile_time_data_header: () =>
      token(prec(3, choice(
        seq(/\*\*[Cc][Tt][Dd][Aa][Tt][Aa]/, /[^\r\n]*/, /\r?\n/),
        seq("**", /[ \t]*/, optional(/[A-Za-z_][A-Za-z0-9_]*/), /[ \t]*/, /\r?\n/),
      ))),

    compile_time_data_value: () =>
      token(prec(2, seq(/[^\r\n]+/, /\r?\n/))),

    procedure: ($) =>
      seq(
        field("start", $.procedure_start),
        repeat($._body_item),
        field("end", $.procedure_end),
      ),

    procedure_start: ($) =>
      seq(
        alias(caseInsensitive("DCL-PROC"), $.keyword),
        field("name", $.identifier),
        repeat($._statement_atom),
        alias($.terminator, ";"),
      ),

    procedure_end: ($) =>
      seq(
        alias(caseInsensitive("END-PROC"), $.keyword),
        optional(field("name", $.identifier)),
        alias($.terminator, ";"),
      ),

    prototype: ($) =>
      seq(
        field("start", $.prototype_start),
        repeat($.parameter_declaration),
        field("end", $.prototype_end),
      ),

    prototype_start: ($) =>
      seq(
        alias(caseInsensitive("DCL-PR"), $.keyword),
        field("name", $._name),
        repeat($._statement_atom),
        alias($.terminator, ";"),
      ),

    prototype_end: ($) =>
      seq(alias(caseInsensitive("END-PR"), $.keyword), alias($.terminator, ";")),

    procedure_interface: ($) =>
      seq(
        field("start", $.interface_start),
        repeat($.parameter_declaration),
        field("end", $.interface_end),
      ),

    interface_start: ($) =>
      seq(
        alias(caseInsensitive("DCL-PI"), $.keyword),
        field("name", $._name),
        repeat($._statement_atom),
        alias($.terminator, ";"),
      ),

    interface_end: ($) =>
      seq(alias(caseInsensitive("END-PI"), $.keyword), alias($.terminator, ";")),

    data_structure: ($) =>
      seq(
        field("start", $.data_structure_start),
        repeat($.subfield_declaration),
        field("end", $.data_structure_end),
      ),

    data_structure_start: ($) =>
      seq(
        alias(caseInsensitive("DCL-DS"), $.keyword),
        field("name", $._name),
        repeat($._statement_atom),
        alias($.terminator, ";"),
      ),

    data_structure_end: ($) =>
      seq(alias(caseInsensitive("END-DS"), $.keyword), alias($.terminator, ";")),

    enumeration: ($) =>
      seq(
        field("start", $.enumeration_start),
        repeat($.enumeration_member),
        field("end", $.enumeration_end),
      ),

    enumeration_start: ($) =>
      seq(
        alias(caseInsensitive("DCL-ENUM"), $.keyword),
        field("name", $.identifier),
        repeat($._statement_atom),
        alias($.terminator, ";"),
      ),

    enumeration_member: ($) =>
      seq(field("name", $.identifier), repeat($._statement_atom), alias($.terminator, ";")),

    enumeration_end: ($) =>
      seq(alias(caseInsensitive("END-ENUM"), $.keyword), alias($.terminator, ";")),

    declaration_statement: ($) =>
      choice($.control_statement, $.named_declaration),

    control_statement: ($) =>
      seq(
        field("kind", alias(caseInsensitive("CTL-OPT"), $.declaration_keyword)),
        repeat(field("attribute", $._statement_atom)),
        alias($.terminator, ";"),
      ),

    named_declaration: ($) =>
      seq(
        field("kind", $.declaration_keyword),
        field("name", $._name),
        repeat(field("attribute", $._statement_atom)),
        alias($.terminator, ";"),
      ),

    declaration_keyword: () =>
      token(
        prec(
          3,
          choice(
            caseInsensitive("DCL-C"),
            caseInsensitive("DCL-F"),
            caseInsensitive("DCL-S"),
            caseInsensitive("DCL-PARM"),
            caseInsensitive("DCL-SUBF"),
          ),
        ),
      ),

    parameter_declaration: ($) =>
      seq(
        optional(alias(caseInsensitive("DCL-PARM"), $.keyword)),
        field("name", $._name),
        repeat($._statement_atom),
        alias($.terminator, ";"),
      ),

    subfield_declaration: ($) =>
      seq(
        optional(alias(caseInsensitive("DCL-SUBF"), $.keyword)),
        field("name", $.identifier),
        repeat($._statement_atom),
        alias($.terminator, ";"),
      ),

    _body_item: ($) =>
      choice(
        $.fixed_sql_statement,
        $._fixed_line,
        $.compiler_directive,
        $.procedure_interface,
        $.prototype,
        $.data_structure,
        $.enumeration,
        $.declaration_statement,
        $.if_statement,
        $.do_statement,
        $.for_statement,
        $.select_statement,
        $.monitor_statement,
        $.subroutine,
        $.embedded_sql_statement,
        $.assignment_statement,
        $.expression_statement,
        $.operation_statement,
      ),

    embedded_sql_statement: ($) =>
      seq(
        field("exec", alias(caseInsensitive("EXEC"), $.keyword)),
        field("language", alias(caseInsensitive("SQL"), $.keyword)),
        field("body", $.sql_body),
        alias($.terminator, ";"),
      ),

    sql_body: ($) =>
      repeat1(
        choice(
          $.sql_string,
          $.sql_quoted_identifier,
          $.sql_line_comment,
          $.sql_block_comment,
          $.sql_text,
        ),
      ),

    sql_string: () => token(seq("'", repeat(choice(/[^']+/, "''")), "'")),

    sql_quoted_identifier: () =>
      token(seq('"', repeat(choice(/[^"]+/, '""')), '"')),

    sql_line_comment: () =>
      token(
        prec(
          4,
          choice(
            seq("--", /[^\r\n]*/, optional(/\r?\n/)),
            seq("//", /[^\r\n]*/, optional(/\r?\n/)),
          ),
        ),
      ),

    sql_block_comment: () =>
      token(
        prec(4, seq("/*", /[^*]*\*+([^/*][^*]*\*+)*/, "/")),
      ),

    sql_text: () =>
      token(prec(-2, choice(/[^;'"\/-]+/, "-", "/"))),

    if_statement: ($) =>
      seq(
        alias(caseInsensitive("IF"), $.keyword),
        field("condition", $.expression),
        alias($.terminator, ";"),
        repeat($._body_item),
        repeat($.elseif_clause),
        optional($.else_clause),
        alias(caseInsensitive("ENDIF"), $.keyword),
        alias($.terminator, ";"),
      ),

    elseif_clause: ($) =>
      seq(
        alias(caseInsensitive("ELSEIF"), $.keyword),
        field("condition", $.expression),
        alias($.terminator, ";"),
        repeat($._body_item),
      ),

    else_clause: ($) =>
      seq(alias(caseInsensitive("ELSE"), $.keyword), alias($.terminator, ";"), repeat($._body_item)),

    do_statement: ($) =>
      seq(
        field("kind", alias(choice(caseInsensitive("DOW"), caseInsensitive("DOU")), $.keyword)),
        field("condition", $.expression),
        alias($.terminator, ";"),
        repeat($._body_item),
        alias(caseInsensitive("ENDDO"), $.keyword),
        alias($.terminator, ";"),
      ),

    for_statement: ($) =>
      seq(
        field("kind", alias(choice(caseInsensitive("FOR"), caseInsensitive("FOR-EACH")), $.keyword)),
        repeat($._statement_atom),
        alias($.terminator, ";"),
        repeat($._body_item),
        alias(caseInsensitive("ENDFOR"), $.keyword),
        alias($.terminator, ";"),
      ),

    select_statement: ($) =>
      seq(
        alias(caseInsensitive("SELECT"), $.keyword),
        optional(field("value", $.expression)),
        alias($.terminator, ";"),
        repeat1(choice($.when_clause, $.other_clause)),
        alias(caseInsensitive("ENDSL"), $.keyword),
        alias($.terminator, ";"),
      ),

    when_clause: ($) =>
      seq(
        alias(choice(caseInsensitive("WHEN"), caseInsensitive("WHEN-IS"), caseInsensitive("WHEN-IN")), $.keyword),
        field("condition", $.expression),
        alias($.terminator, ";"),
        repeat($._body_item),
      ),

    other_clause: ($) =>
      seq(alias(caseInsensitive("OTHER"), $.keyword), alias($.terminator, ";"), repeat($._body_item)),

    monitor_statement: ($) =>
      seq(
        alias(caseInsensitive("MONITOR"), $.keyword),
        alias($.terminator, ";"),
        repeat($._body_item),
        repeat1($.handler_clause),
        alias(caseInsensitive("ENDMON"), $.keyword),
        alias($.terminator, ";"),
      ),

    handler_clause: ($) =>
      seq(
        alias(choice(caseInsensitive("ON-ERROR"), caseInsensitive("ON-EXCP"), caseInsensitive("ON-EXIT")), $.keyword),
        repeat($._statement_atom),
        alias($.terminator, ";"),
        repeat($._body_item),
      ),

    subroutine: ($) =>
      seq(
        alias(caseInsensitive("BEGSR"), $.keyword),
        field("name", $.identifier),
        alias($.terminator, ";"),
        repeat($._body_item),
        alias(caseInsensitive("ENDSR"), $.keyword),
        optional(field("name", $.identifier)),
        alias($.terminator, ";"),
      ),

    assignment_statement: ($) =>
      prec.dynamic(2, seq(
        optional(field("opcode", alias(choice(caseInsensitive("EVAL"), caseInsensitive("EVALR"), caseInsensitive("EVAL-CORR")), $.keyword))),
        optional(field("extender", $.opcode_extender)),
        field("left", $._assignable),
        field("operator", $.assignment_operator),
        field("right", $.expression),
        alias($.terminator, ";"),
      )),

    operation_statement: ($) =>
      seq(
        field("opcode", $.opcode),
        optional(field("extender", $.opcode_extender)),
        repeat(field("argument", $._statement_atom)),
        alias($.terminator, ";"),
      ),

    expression_statement: ($) => seq($.call_expression, alias($.terminator, ";")),

    opcode: ($) => $.identifier,

    opcode_extender: () => token(seq("(", /[A-Za-z]+/, ")")),

    _statement_atom: ($) =>
      choice(
        $.compiler_directive,
        $.expression,
        $.colon,
      ),

    expression: ($) =>
      choice(
        $.binary_expression,
        $.unary_expression,
        $.call_expression,
        $.member_expression,
        $.parenthesized_expression,
        $.known_builtin_name,
        $.builtin_name,
        $.special_value,
        $.string,
        $.hex_string,
        $.number,
        $.identifier,
      ),

    binary_expression: ($) => {
      const table = [
        [1, ["OR"]],
        [2, ["AND"]],
        [4, ["+", "-"]],
        [5, ["*", "/"]],
        [6, ["**"]],
      ];
      return choice(
        prec.left(
          3,
          seq(
            field("left", $.expression),
            field("operator", alias($.comparison_operator, $.operator)),
            field("right", $.expression),
          ),
        ),
        ...table.map(([precedence, operators]) =>
          prec.left(
            precedence,
            seq(
              field("left", $.expression),
              field(
                "operator",
                alias(
                  operators.length === 1
                    ? operatorToken(operators[0])
                    : choice(...operators.map(operatorToken)),
                  $.operator,
                ),
              ),
              field("right", $.expression),
            ),
          ),
        ),
      );
    },

    comparison_operator: () =>
      token(prec(-1, choice("=", "<>", ">", ">=", "<", "<="))),

    unary_expression: ($) =>
      prec(7, seq(field("operator", alias(choice(caseInsensitive("NOT"), "+", "-"), $.operator)), field("argument", $.expression))),

    call_expression: ($) =>
      prec(9, seq(field("function", choice($.identifier, $.known_builtin_name, $.builtin_name, $.special_value, $.member_expression)), field("arguments", $.argument_list))),

    member_expression: ($) =>
      prec.left(10, seq(field("object", $.expression), ".", field("member", $.identifier))),

    parenthesized_expression: ($) => seq("(", $.expression, ")"),

    argument_list: ($) => seq("(", optional(seq($.expression, repeat(seq(":", $.expression)))), ")"),

    known_builtin_name: () =>
      token(prec(2, choice(...BUILTINS.map((name) => caseInsensitive(`%${name}`))))),

    builtin_name: () => token(prec(1, /%[A-Za-z][A-Za-z0-9_]*/)),

    _assignable: ($) => choice($.identifier, $.special_value, $.call_expression, $.member_expression),

    assignment_operator: () =>
      token(prec(3, choice("=", "+=", "-=", "*=", "/=", "**="))),

    _name: ($) => choice($.identifier, $.special_value),

    identifier: () => token(/[A-Za-z_#$@§][A-Za-z0-9_#$@§-]*/),

    special_value: () => token(/\*[A-Za-z0-9_]+[./]?('[^']*')?/),

    number: () => token(/[0-9]+(\.[0-9]+)?/),

    string: () => token(seq("'", repeat(choice(/[^']+/, "''")), "'")),

    hex_string: () => token(/[Xx]'[0-9A-Fa-f]*'/),

    colon: () => ":",

    terminator: () => token(seq(";", optional(/[ \t]*\r?\n/))),

    comment: () => token(seq("//", /[^\r\n]*/, optional(/\r?\n/))),
  },
});

function caseInsensitive(value) {
  return new RegExp(
    value
      .split("")
      .map((character) =>
        /[A-Za-z]/.test(character)
          ? `[${character.toUpperCase()}${character.toLowerCase()}]`
          : /[\\^$.*+?()[\]{}|/-]/.test(character)
            ? `\\${character}`
            : character,
      )
      .join(""),
  );
}

function operatorToken(value) {
  return /^[A-Z]+$/.test(value) ? caseInsensitive(value) : value;
}

function optionalNested($, fields) {
  const [[name, rule], ...rest] = fields;
  const value = name ? field(name, rule) : rule;
  return rest.length === 0
    ? value
    : seq(value, optional(optionalNested($, rest)));
}
