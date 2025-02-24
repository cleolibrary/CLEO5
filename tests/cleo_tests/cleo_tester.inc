
const VAR_TEST_INDEX = 0
const VAR_BEFORE_EACH = 1
const VAR_AFTER_EACH = 2
const VAR_SPEC = 3
const VAR_ASSERT_INDEX = 4
const VAR_COMPAT = 5

const CLEO5_COMPAT = 0x05000000
const CLEO4_COMPAT = 0x04040000
const DEFAULT_COMPAT = CLEO5_COMPAT

jump @_cleo_tester_skip_fns

function _cleo_tester_read_var(index: int): int
    int buf = get_label_pointer @_cleo_tester_shared_vars
    index *= 4
    int value = read_memory_with_offset {address} buf {offset} index {size} 4
    return value
end

function _cleo_tester_write_var(index: int, value: int)
    int buf = get_label_pointer @_cleo_tester_shared_vars
    index *= 4
    write_memory_with_offset {address} buf {offset} index {size} 4 {value} value
end

/// registers new test suite (collection of unit tests)
/// use it(...) for individual unit tests
function test(suite_name: string, callback: int)
    debug_on

    int suite_name_buf = get_label_pointer @_cleo_tester_test_name
    copy_memory {src} suite_name {dest} suite_name_buf {size} 255 // used in an it trace
    trace "" // separator
    trace "Testing %s" suite_name

    _cleo_tester_write_var(VAR_BEFORE_EACH, @_cleo_tester_stub)
    _cleo_tester_write_var(VAR_AFTER_EACH, @_cleo_tester_stub)
    _cleo_tester_write_var(VAR_TEST_INDEX, 1)
    cleo_call callback
end

/// registers new unit test in a test suite
/// use assert_*(...) to validate result
function it(spec_name: string, callback: int)
    _run_with_compat_mode(spec_name, callback, DEFAULT_COMPAT)
end

/// registers new unit test in a test suite - runs in CS4 mode
/// use assert_*(...) to validate result
function it_cs4(spec_name: string, callback: int)
    _run_with_compat_mode(spec_name, callback, CLEO4_COMPAT)
end

/// registers new unit test in a test suite
/// use assert_*(...) to validate result
function _run_with_compat_mode(spec_name: string, callback: int, compat: int)
    int index = _cleo_tester_read_var(VAR_TEST_INDEX)
    _cleo_tester_write_var(VAR_COMPAT, compat)

    int spec_name_buf = get_label_pointer @_cleo_tester_spec_name
    copy_memory {src} spec_name {dest} spec_name_buf {size} 255 // used in a failed assert
    int test_name = get_label_pointer @_cleo_tester_test_name

    if compat == DEFAULT_COMPAT
    then
        trace "~s~Test #%d %s" index spec_name
    else
        trace "~s~Test #%d %s - COMPAT MODE %d" index spec_name compat
    end

    wait 0
    _cleo_tester_write_var(VAR_SPEC, callback)
    _cleo_tester_write_var(VAR_ASSERT_INDEX, 0)

    run_spec()

    //trace "~g~~h~~h~Test #%d PASSED" index
    index++
    _cleo_tester_write_var(VAR_TEST_INDEX, index)

    return

    function run_spec
        // this function should use 0 local variables

        inject_offset(@_cleo_tester_before_each, VAR_BEFORE_EACH)
        inject_offset(@_cleo_tester_after_each, VAR_AFTER_EACH)
        inject_offset(@_cleo_tester_run, VAR_SPEC)

        :_cleo_tester_before_each
        gosub @_cleo_tester_stub

        _set_current_script_compat()
        :_cleo_tester_run
        cleo_call @_cleo_tester_stub {numParams} 32 {params} 0@ 1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15@ 16@ 17@ 18@ 19@ 20@ 21@ 22@ 23@ 24@ 25@ 26@ 27@ 28@ 29@ 30@ 31@
        _restore_current_script_compat()

        :_cleo_tester_after_each
        gosub @_cleo_tester_stub
    end

    // self-patch script code with new gosub/call offset
    function inject_offset(label: int, var_index: int)
        int function_offset = _cleo_tester_read_var(var_index)
        int location = get_label_pointer label
        write_memory_with_offset {address} location {offset - opcode + datatype} 3 {size} 4 {value} function_offset
    end
end

:_cleo_tester_stub
return

:_cleo_tester_shared_vars
hex
    00(24) // 6 variables
end
:_cleo_tester_test_name
hex
    00(256)
end
:_cleo_tester_spec_name
hex
    00(256)
end

/// skips unit test in a test suite
function xit(spec_name: string, callback: int)
    int index = _cleo_tester_read_var(VAR_TEST_INDEX)
    trace "Skipping test #%d %s" index spec_name
    index++
    _cleo_tester_write_var(VAR_TEST_INDEX, index)
end

function _cleo_tester_fail
    int test_index = _cleo_tester_read_var(VAR_TEST_INDEX)
    int test_name = get_label_pointer @_cleo_tester_spec_name
    int assert_index = _cleo_tester_read_var(VAR_ASSERT_INDEX)
    trace "~r~~h~~h~Test #%d ~p~~h~Assert #%d~r~~h~~h~ FAILED!" test_index assert_index
end

function _cleo_tester_increment_assert
    int index = _cleo_tester_read_var(VAR_ASSERT_INDEX)
    index++
    _cleo_tester_write_var(VAR_ASSERT_INDEX, index)
end

/// checks if bool value is true (different than 0)
function assert_true(flag: int)
    _cleo_tester_increment_assert()
    if
        flag == false
    then
        _cleo_tester_fail()
        trace "~g~~h~~h~TRUE~s~ expected~n~~r~~h~~h~%d~s~ occured" flag
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if bool value is false
function assert_false(flag: int)
    _cleo_tester_increment_assert()
    if
        flag <> false
    then
        _cleo_tester_fail()
        trace "~g~~h~~h~FALSE~s~ expected~n~~r~~h~~h~%d~s~ occured" flag
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if condition result value is true
:assert_result_true
goto_if_false @_assert_result_true
_cleo_tester_increment_assert()
return

:_assert_result_true
_cleo_tester_increment_assert()
_cleo_tester_fail()
trace "~s~Condition result is ~r~~h~~h~FALSE~s~, expected ~g~~h~~h~TRUE~s~"
breakpoint
terminate_this_custom_script
return

/// checks if condition result value is false
:assert_result_false
goto_if_false @_assert_result_false

_cleo_tester_increment_assert()
_cleo_tester_fail()
trace "~s~Condition result is ~r~~h~~h~TRUE~s~, expected ~g~~h~~h~FALSE~s~"
breakpoint
terminate_this_custom_script

:_assert_result_false
_cleo_tester_increment_assert()
return

/// checks if two int values are equal, otherwise stops the test execution
function assert_eq(actual: int, expected: int)
    _cleo_tester_increment_assert()
    if
        actual <> expected
    then
        _cleo_tester_fail()
        trace "~g~~h~~h~%08X~s~ expected~n~~r~~h~~h~%08X~s~ occured" expected actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if two int values are not equal, otherwise stops the test execution
function assert_neq(actual: int, expected: int)
    if
        actual == expected
    then
        _cleo_tester_fail()
        trace "~s~Expected value different than ~r~~h~~h~%08X~s~" actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if int value is within specified range, otherwise stops the test execution
function assert_range(actual: int, expectedMin: int, expectedMax: int)
    _cleo_tester_increment_assert()
    if or
        actual < expectedMin
        actual > expectedMax
    then
        _cleo_tester_fail()
        trace "~g~~h~~h~%08X to %08X~s~ expected~n~~r~~h~~h~%08X~s~ occured" expectedMin expectedMax actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if two float values are equal, otherwise stops the test execution
function assert_eqf(actual: float, expected: float)
    _cleo_tester_increment_assert()
    if
        actual <> expected
    then
        _cleo_tester_fail()
        trace "~g~~h~~h~%f~s~ expected~n~~r~~h~~h~%f~s~ occured" expected actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if two float values are not equal, otherwise stops the test execution
function assert_neqf(actual: float, expected: float)
    _cleo_tester_increment_assert()
    if
        actual == expected
    then
        _cleo_tester_fail()
        trace "~s~Expected value different than ~r~~h~~h~%f~s~" actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if float value is within specified range, otherwise stops the test execution
function assert_rangef(actual: float, expectedMin: float, expectedMax: float)
    _cleo_tester_increment_assert()
    if or
        actual < expectedMin
        actual > expectedMax
    then
        _cleo_tester_fail()
        trace "~g~~h~~h~%f to %f~s~ expected~n~~r~~h~~h~%f~s~ occured" expectedMin expectedMax actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if value is a valid pointer, otherwise stops the test execution
function assert_ptr(ptr: int)
    _cleo_tester_increment_assert()
    if
        ptr <= 0x10000 // possibly valid pointer
    then
        _cleo_tester_fail()
        trace "%08X is not valid pointer" ptr
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if value is not 0, otherwise stops the test execution
function assert(flag: int)
    _cleo_tester_increment_assert()
    if
        flag == 0
    then
        _cleo_tester_fail()
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if two string values are equal, otherwise stops the test execution
function assert_eqs(actual: string, expected: string)
    _cleo_tester_increment_assert()
    if
        not is_text_equal {text} actual {another} expected {ignoreCase} false
    then
        _cleo_tester_fail()
        trace "`~g~~h~~h~%s~s~` expected~n~`~r~~h~~h~%s~s~` occured" expected actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if two string values are not equal, otherwise stops the test execution
function assert_neqs(actual: string, expected: string)
    _cleo_tester_increment_assert()
    if
        is_text_equal {text} actual {another} expected {ignoreCase} false
    then
        _cleo_tester_fail()
        trace "~s~Expected value different than `~r~~h~~h~%s~s~`" actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if string starts with prefix, otherwise stops the test execution
function assert_prefix(actual: string, expectedPrefix: string, ignoreCase: int)
    _cleo_tester_increment_assert()
    if
        not is_text_prefix {text} actual {prefix} expectedPrefix {ignoreCase} ignoreCase
    then
        _cleo_tester_fail()
        trace "`~g~~h~~h~%s~s~` prefix expected~n~`~r~~h~~h~%s~s~` text occured" expectedPrefix actual
        breakpoint
        terminate_this_custom_script
    end
end

/// checks if string ends with suffix, otherwise stops the test execution
function assert_suffix(actual: string, expectedSuffix: string, ignoreCase: int)
    _cleo_tester_increment_assert()
    if
        not is_text_suffix {text} actual {prefix} expectedSuffix {ignoreCase} ignoreCase
    then
        _cleo_tester_fail()
        trace "`~g~~h~~h~%s~s~` suffix expected~n~`~r~~h~~h~%s~s~` text occured" expectedSuffix actual
        breakpoint
        terminate_this_custom_script
    end
end

/// registers a callback that runs before each unit test (test setup)
function before_each(callback: int)
    _cleo_tester_write_var(VAR_BEFORE_EACH, callback)
end

/// registers a callback that runs after each unit test (test teardown)
function after_each(callback: int)
    _cleo_tester_write_var(VAR_AFTER_EACH, callback)
end

function _set_current_script_compat_to(compat: int)
    int lib, proc, ver
    if
        lib = load_dynamic_library "root:\CLEO.asi"
    then
        if
            proc = get_dynamic_library_procedure "_CLEO_SetScriptVersion@8" {library} lib
        then
            int cur_script = get_this_script_struct
            call_function {address} proc {numParams} 2 {pop} 0 {funcParams} compat cur_script
        else
            trace "Can't find SetScriptVersion in CLEO.asi"
            breakpoint
        end

        free_dynamic_library lib
    else
        trace "Can't load CLEO.asi"
        breakpoint
    end
end

function _set_current_script_compat
    int compat = _cleo_tester_read_var(VAR_COMPAT)
    _set_current_script_compat_to(compat)
end

function _restore_current_script_compat
    _set_current_script_compat_to(DEFAULT_COMPAT)
end


:_cleo_tester_skip_fns