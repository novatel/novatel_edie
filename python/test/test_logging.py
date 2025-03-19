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

LOG_LEVELS = [
    logging.DEBUG, logging.INFO, logging.WARNING, logging.ERROR, logging.CRITICAL
]

@pytest.fixture(scope='function')
def logger_tester():
    tester = _internal.LoggerTester()
    # Reset the log configuration to a default state
    enable_internal_logging()
    logging.getLogger().setLevel(logging.DEBUG)
    logging.getLogger('novatel_edie').setLevel(logging.NOTSET)
    logging.getLogger('novatel_edie.logger_tester').setLevel(logging.NOTSET)
    return tester

def test_example_log(logger_tester, caplog):
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

def test_toggle_internal_logging(logger_tester, caplog):
    # Act
    disable_internal_logging()
    logger_tester.LogInfo('No log message')
    enable_internal_logging()
    logger_tester.LogInfo('Log message')
    # Assert
    assert [record.message for record in caplog.records] == ['Log message']

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
