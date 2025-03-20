################################################################################
#
# COPYRIGHT NovAtel Inc, 2022. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
################################################################################
#                            DESCRIPTION
#
# \brief Unit tests for Logging.
################################################################################

import logging

import pytest

from novatel_edie import _internal, enable_internal_logging, disable_internal_logging

# pylint: disable=redefined-outer-name

LOG_LEVELS = [
    logging.DEBUG, logging.INFO, logging.WARNING, logging.ERROR, logging.CRITICAL
]

@pytest.fixture(scope='function')
def logger_tester():
    """A helper object for logging within C++ code."""
    tester = _internal.LoggerTester()
    # Reset the log configuration to a default state
    enable_internal_logging()
    logging.getLogger().setLevel(logging.DEBUG)
    logging.getLogger('novatel_edie').setLevel(logging.NOTSET)
    logging.getLogger('novatel_edie.logger_tester').setLevel(logging.NOTSET)
    return tester

def test_example_log(logger_tester, caplog):
    """Tests that an example log message is logged correctly."""
    # Act
    logger_tester.LogInfo('Info message')
    # Assert
    record = caplog.records[0]
    assert record.levelno == logging.INFO
    assert record.message == 'Info message'
    assert record.name == 'novatel_edie.logger_tester'
    assert record.filename == 'logger_tester.cpp'
    assert isinstance(record.lineno, int)
    assert record.funcName.endswith('LogInfo') # Namespace may also be present

@pytest.mark.parametrize('config_point', [
    'root', 'novatel_edie', 'novatel_edie.logger_tester'])
@pytest.mark.parametrize('log_level', LOG_LEVELS)
def test_log_level_config(log_level, config_point, logger_tester, caplog):
    """Tests that the correct messages are logged."""
    # Arrange
    logging.getLogger(config_point).setLevel(log_level)
    exp_log_levels = [level for level in LOG_LEVELS if level >= log_level]
    # Act
    logger_tester.LogDebug('Debug message')
    logger_tester.LogInfo('Info message')
    logger_tester.LogWarn('Warn message')
    logger_tester.LogError('Error message')
    logger_tester.LogCritical('Critical message')
    # Assert
    assert [rec.levelno for rec in caplog.records] == exp_log_levels

@pytest.mark.parametrize('config_point', [
    'root', 'novatel_edie', 'novatel_edie.logger_tester'])
def test_toggle_logging_level(config_point, logger_tester, caplog):
    """Tests that log level can be toggled back and forth appropriately."""
    logger = logging.getLogger(config_point)
    # Act
    logger.setLevel(logging.WARNING)
    logger_tester.LogInfo('No log message')
    logger.setLevel(logging.DEBUG)
    logger_tester.LogInfo('Log message')
    # Assert
    assert [record.message for record in caplog.records] == ['Log message']

@pytest.mark.parametrize('base_logger_point', ['root', 'novatel_edie'])
@pytest.mark.parametrize('log_level', [logging.NOTSET] + LOG_LEVELS)
def test_overriding_log_levels(log_level, base_logger_point, logger_tester, caplog):
    """Tests that log levels inherit from parent only when not set."""
    # Arrange
    base_level = logging.WARNING
    logging.getLogger('novatel_edie.logger_tester').setLevel(log_level)
    logging.getLogger(base_logger_point).setLevel(base_level)
    exp_level = log_level if log_level else base_level
    exp_log_levels = [level for level in LOG_LEVELS if level >= exp_level]
    # Act
    logger_tester.LogDebug('Debug message')
    logger_tester.LogInfo('Info message')
    logger_tester.LogWarn('Warn message')
    logger_tester.LogError('Error message')
    logger_tester.LogCritical('Critical message')
    # Assert
    assert [rec.levelno for rec in caplog.records] == exp_log_levels

def test_toggle_internal_logging(logger_tester, caplog):
    """Tests turning internal logging off and on has the intended effect."""
    # Act
    disable_internal_logging()
    logging.getLogger().setLevel(logging.DEBUG)
    logger_tester.LogInfo('No log message')
    enable_internal_logging()
    logger_tester.LogInfo('Log message')
    # Assert
    assert [record.message for record in caplog.records] == ['Log message']

@pytest.mark.parametrize('logger_name', ['root', 'novatel_edie', 'novatel_edie.logger_tester'])
@pytest.mark.parametrize('func_input, exp_result', [
    ('INVALID', ValueError), ('DEBUG', logging.DEBUG), ('INFO', logging.INFO),
    ('WARNING', logging.WARNING), ('WARN', logging.WARN), ('ERROR', logging.ERROR),
    ('CRITICAL', logging.CRITICAL), ('NOTSET', logging.NOTSET),
    (32.31, TypeError), ((logging.ERROR, 'ERROR'), TypeError)
])
def test_set_level(func_input, logger_name, exp_result, logger_tester):
    """Tests default behavior of setLevel is not impacted by update hook."""
    # Arrange
    func_input = (func_input,) if not isinstance(func_input, tuple) else func_input
    logger = logging.getLogger(logger_name)
    # Act and Assert
    if isinstance(exp_result, type) and issubclass(exp_result, Exception):
        with pytest.raises(exp_result):
            logger.setLevel(*func_input)
    else:
        logger.setLevel(*func_input)
        assert logger.level == exp_result
