import numpy as np
import pytest

import taichi as ti

if ti.has_pytorch():
    import torch


@pytest.mark.parametrize('size', [[1], [1, 2, 3, 4]])
@ti.test()
def test_get_external_tensor_shape_access_numpy(size):
    @ti.kernel
    def func(x: ti.ext_arr(), index: ti.template()) -> ti.i32:
        return x.shape[index]

    x_hat = np.ones(size, dtype=np.int32)
    for idx, y_ref in enumerate(size):
        y_hat = func(x_hat, idx)
        assert y_ref == y_hat, "Size of axis {} should equal {} and not {}.".format(
            idx, y_ref, y_hat)


@pytest.mark.parametrize('size', [[1, 1], [2, 2]])
@ti.test()
def test_get_external_tensor_shape_sum_numpy(size):
    @ti.kernel
    def func(x: ti.ext_arr()) -> ti.i32:
        y = 0
        for i in range(x.shape[0]):
            for j in range(x.shape[1]):
                y += x[i, j]
        return y

    x_hat = np.ones(size, dtype=np.int32)
    x_ref = x_hat
    y_hat = func(x_hat)
    y_ref = x_ref.sum()
    assert y_ref == y_hat, "Output should equal {} and not {}.".format(
        y_ref, y_hat)


@pytest.mark.skipif(not ti.has_pytorch(), reason='Pytorch not installed.')
@pytest.mark.parametrize('size', [[1, 2, 3, 4]])
@ti.test(exclude=ti.opengl)
def test_get_external_tensor_shape_access_torch(size):
    @ti.kernel
    def func(x: ti.ext_arr(), index: ti.template()) -> ti.i32:
        return x.shape[index]

    x_hat = torch.ones(size, dtype=torch.int32, device='cpu')
    for idx, y_ref in enumerate(size):
        y_hat = func(x_hat, idx)
        assert y_ref == y_hat, "Size of axis {} should equal {} and not {}.".format(
            idx, y_ref, y_hat)


@pytest.mark.skipif(not ti.has_pytorch(), reason='Pytorch not installed.')
@pytest.mark.parametrize('size', [[1, 2, 3, 4]])
@ti.test(arch=[ti.cpu, ti.cuda, ti.opengl])
def test_get_external_tensor_shape_access_ndarray(size):
    @ti.kernel
    def func(x: ti.any_arr(), index: ti.template()) -> ti.i32:
        return x.shape[index]

    x_hat = ti.ndarray(ti.i32, shape=size)
    for idx, y_ref in enumerate(size):
        y_hat = func(x_hat, idx)
        assert y_ref == y_hat, "Size of axis {} should equal {} and not {}.".format(
            idx, y_ref, y_hat)
