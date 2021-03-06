import sys
sys.path.insert(0, '../common_python')
import tools
import pytest
import os
import common_code

def skeleton_mnist_debug(cluster, dir_name, executables, compiler_name, weekly, debug, should_log=False):
    # If weekly or debug are true, then run the test.
    if (not weekly) and (not debug):
        pytest.skip('Not doing weekly or debug testing')
    if compiler_name not in executables:
      pytest.skip('default_exes[%s] does not exist' % compiler_name)
    model_name = 'lenet_mnist'
    output_file_name = '%s/bamboo/integration_tests/output/%s_%s_output.txt' %(dir_name, model_name, compiler_name)
    error_file_name = '%s/bamboo/integration_tests/error/%s_%s_error.txt' %(dir_name, model_name, compiler_name)
    command = tools.get_command(
        cluster=cluster, executable=executables[compiler_name], num_nodes=1,
        partition='pbatch', time_limit=100, dir_name=dir_name,
        data_filedir_ray='/p/gscratchr/brainusr/datasets/MNIST',
        data_reader_name='mnist', model_folder='models/' + model_name,
        model_name=model_name, num_epochs=5, optimizer_name='adagrad',
        output_file_name=output_file_name, error_file_name=error_file_name)
    output_value = common_code.run_lbann(command, model_name, output_file_name, error_file_name)
    assert output_value == 0

def skeleton_cifar_debug(cluster, dir_name, executables, compiler_name, weekly, debug, should_log=False):
    # If weekly or debug are true, then run the test.                                                                                                                   
    if (not weekly) and (not debug):
        pytest.skip('Not doing weekly or debug testing')
    if cluster == 'ray':
        pytest.skip('cifar not operational on Ray')
    if compiler_name not in executables:
      pytest.skip('default_exes[%s] does not exist' % compiler_name)
    model_name = 'autoencoder_cifar10'
    output_file_name = '%s/bamboo/integration_tests/output/%s_%s_output.txt' %(dir_name, model_name, compiler_name)
    error_file_name = '%s/bamboo/integration_tests/error/%s_%s_error.txt' %(dir_name, model_name, compiler_name)
    command = tools.get_command(
        cluster=cluster, executable=executables[compiler_name],	num_nodes=1,
        partition='pbatch', time_limit=100, dir_name=dir_name,
        data_reader_name='cifar10', data_reader_percent=0.01, model_folder='models/' + model_name,
        model_name='conv_' + model_name, num_epochs=5, optimizer_name='adagrad',
        output_file_name=output_file_name, error_file_name=error_file_name)
    output_value = common_code.run_lbann(command, model_name, output_file_name, error_file_name)
    assert output_value == 0

def test_integration_mnist_clang4_debug(cluster, dirname, exes, weekly, debug):
    skeleton_mnist_debug(cluster, dirname, exes, 'clang4_debug', weekly, debug)

def test_integration_mnist_gcc4_debug(cluster, dirname, exes, weekly, debug):
    skeleton_mnist_debug(cluster, dirname, exes, 'gcc4_debug', weekly, debug)

def test_integration_mnist_gcc7_debug(cluster, dirname, exes, weekly, debug):
    skeleton_mnist_debug(cluster, dirname, exes, 'gcc7_debug', weekly, debug)

def test_integration_mnist_intel18_debug(cluster, dirname, exes, weekly, debug):
    skeleton_mnist_debug(cluster, dirname, exes, 'intel18_debug', weekly, debug)

def test_integration_cifar_clang4_debug(cluster, dirname, exes, weekly, debug):
    skeleton_cifar_debug(cluster, dirname, exes, 'clang4_debug', weekly, debug)

def test_integration_cifar_gcc4_debug(cluster, dirname, exes, weekly, debug):
    skeleton_cifar_debug(cluster, dirname, exes, 'gcc4_debug', weekly, debug)

def test_integration_cifar_gcc7_debug(cluster, dirname, exes, weekly, debug):
    skeleton_cifar_debug(cluster, dirname, exes, 'gcc7_debug', weekly, debug)

def test_integration_cifar_intel18_debug(cluster, dirname, exes, weekly, debug):
    skeleton_cifar_debug(cluster, dirname, exes, 'intel18_debug', weekly, debug)
