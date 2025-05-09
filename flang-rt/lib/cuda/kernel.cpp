//===-- lib/cuda/kernel.cpp -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "flang/Runtime/CUDA/kernel.h"
#include "flang-rt/runtime/descriptor.h"
#include "flang-rt/runtime/terminator.h"
#include "flang/Runtime/CUDA/common.h"

#include "cuda_runtime.h"

extern "C" {

void RTDEF(CUFLaunchKernel)(const void *kernel, intptr_t gridX, intptr_t gridY,
    intptr_t gridZ, intptr_t blockX, intptr_t blockY, intptr_t blockZ,
    int64_t *stream, int32_t smem, void **params, void **extra) {
  dim3 gridDim;
  gridDim.x = gridX;
  gridDim.y = gridY;
  gridDim.z = gridZ;
  dim3 blockDim;
  blockDim.x = blockX;
  blockDim.y = blockY;
  blockDim.z = blockZ;
  unsigned nbNegGridDim{0};
  if (gridX < 0) {
    ++nbNegGridDim;
  }
  if (gridY < 0) {
    ++nbNegGridDim;
  }
  if (gridZ < 0) {
    ++nbNegGridDim;
  }
  if (nbNegGridDim == 1) {
    int maxBlocks, nbBlocks, dev, multiProcCount;
    cudaError_t err1, err2;
    nbBlocks = blockDim.x * blockDim.y * blockDim.z;
    cudaGetDevice(&dev);
    err1 = cudaDeviceGetAttribute(
        &multiProcCount, cudaDevAttrMultiProcessorCount, dev);
    err2 = cudaOccupancyMaxActiveBlocksPerMultiprocessor(
        &maxBlocks, kernel, nbBlocks, smem);
    if (err1 == cudaSuccess && err2 == cudaSuccess) {
      maxBlocks = multiProcCount * maxBlocks;
    }
    if (maxBlocks > 0) {
      if (gridX > 0) {
        maxBlocks = maxBlocks / gridDim.x;
      }
      if (gridY > 0) {
        maxBlocks = maxBlocks / gridDim.y;
      }
      if (gridZ > 0) {
        maxBlocks = maxBlocks / gridDim.z;
      }
      if (maxBlocks < 1) {
        maxBlocks = 1;
      }
      if (gridX < 0) {
        gridDim.x = maxBlocks;
      }
      if (gridY < 0) {
        gridDim.y = maxBlocks;
      }
      if (gridZ < 0) {
        gridDim.z = maxBlocks;
      }
    }
  } else if (nbNegGridDim > 1) {
    Fortran::runtime::Terminator terminator{__FILE__, __LINE__};
    terminator.Crash("Too many invalid grid dimensions");
  }
  cudaStream_t defaultStream = 0;
  CUDA_REPORT_IF_ERROR(cudaLaunchKernel(kernel, gridDim, blockDim, params, smem,
      stream != nullptr ? (cudaStream_t)(*stream) : defaultStream));
}

void RTDEF(CUFLaunchClusterKernel)(const void *kernel, intptr_t clusterX,
    intptr_t clusterY, intptr_t clusterZ, intptr_t gridX, intptr_t gridY,
    intptr_t gridZ, intptr_t blockX, intptr_t blockY, intptr_t blockZ,
    int64_t *stream, int32_t smem, void **params, void **extra) {
  cudaLaunchConfig_t config;
  config.gridDim.x = gridX;
  config.gridDim.y = gridY;
  config.gridDim.z = gridZ;
  config.blockDim.x = blockX;
  config.blockDim.y = blockY;
  config.blockDim.z = blockZ;
  unsigned nbNegGridDim{0};
  if (gridX < 0) {
    ++nbNegGridDim;
  }
  if (gridY < 0) {
    ++nbNegGridDim;
  }
  if (gridZ < 0) {
    ++nbNegGridDim;
  }
  if (nbNegGridDim == 1) {
    int maxBlocks, nbBlocks, dev, multiProcCount;
    cudaError_t err1, err2;
    nbBlocks = config.blockDim.x * config.blockDim.y * config.blockDim.z;
    cudaGetDevice(&dev);
    err1 = cudaDeviceGetAttribute(
        &multiProcCount, cudaDevAttrMultiProcessorCount, dev);
    err2 = cudaOccupancyMaxActiveBlocksPerMultiprocessor(
        &maxBlocks, kernel, nbBlocks, smem);
    if (err1 == cudaSuccess && err2 == cudaSuccess) {
      maxBlocks = multiProcCount * maxBlocks;
    }
    if (maxBlocks > 0) {
      if (gridX > 0) {
        maxBlocks = maxBlocks / config.gridDim.x;
      }
      if (gridY > 0) {
        maxBlocks = maxBlocks / config.gridDim.y;
      }
      if (gridZ > 0) {
        maxBlocks = maxBlocks / config.gridDim.z;
      }
      if (maxBlocks < 1) {
        maxBlocks = 1;
      }
      if (gridX < 0) {
        config.gridDim.x = maxBlocks;
      }
      if (gridY < 0) {
        config.gridDim.y = maxBlocks;
      }
      if (gridZ < 0) {
        config.gridDim.z = maxBlocks;
      }
    }
  } else if (nbNegGridDim > 1) {
    Fortran::runtime::Terminator terminator{__FILE__, __LINE__};
    terminator.Crash("Too many invalid grid dimensions");
  }
  config.dynamicSmemBytes = smem;
  if (stream != nullptr) {
    config.stream = (cudaStream_t)(*stream);
  } else {
    config.stream = 0;
  }
  cudaLaunchAttribute launchAttr[1];
  launchAttr[0].id = cudaLaunchAttributeClusterDimension;
  launchAttr[0].val.clusterDim.x = clusterX;
  launchAttr[0].val.clusterDim.y = clusterY;
  launchAttr[0].val.clusterDim.z = clusterZ;
  config.numAttrs = 1;
  config.attrs = launchAttr;
  CUDA_REPORT_IF_ERROR(cudaLaunchKernelExC(&config, kernel, params));
}

void RTDEF(CUFLaunchCooperativeKernel)(const void *kernel, intptr_t gridX,
    intptr_t gridY, intptr_t gridZ, intptr_t blockX, intptr_t blockY,
    intptr_t blockZ, int64_t *stream, int32_t smem, void **params,
    void **extra) {
  dim3 gridDim;
  gridDim.x = gridX;
  gridDim.y = gridY;
  gridDim.z = gridZ;
  dim3 blockDim;
  blockDim.x = blockX;
  blockDim.y = blockY;
  blockDim.z = blockZ;
  unsigned nbNegGridDim{0};
  if (gridX < 0) {
    ++nbNegGridDim;
  }
  if (gridY < 0) {
    ++nbNegGridDim;
  }
  if (gridZ < 0) {
    ++nbNegGridDim;
  }
  if (nbNegGridDim == 1) {
    int maxBlocks, nbBlocks, dev, multiProcCount;
    cudaError_t err1, err2;
    nbBlocks = blockDim.x * blockDim.y * blockDim.z;
    cudaGetDevice(&dev);
    err1 = cudaDeviceGetAttribute(
        &multiProcCount, cudaDevAttrMultiProcessorCount, dev);
    err2 = cudaOccupancyMaxActiveBlocksPerMultiprocessor(
        &maxBlocks, kernel, nbBlocks, smem);
    if (err1 == cudaSuccess && err2 == cudaSuccess) {
      maxBlocks = multiProcCount * maxBlocks;
    }
    if (maxBlocks > 0) {
      if (gridX > 0) {
        maxBlocks = maxBlocks / gridDim.x;
      }
      if (gridY > 0) {
        maxBlocks = maxBlocks / gridDim.y;
      }
      if (gridZ > 0) {
        maxBlocks = maxBlocks / gridDim.z;
      }
      if (maxBlocks < 1) {
        maxBlocks = 1;
      }
      if (gridX < 0) {
        gridDim.x = maxBlocks;
      }
      if (gridY < 0) {
        gridDim.y = maxBlocks;
      }
      if (gridZ < 0) {
        gridDim.z = maxBlocks;
      }
    }
  } else if (nbNegGridDim > 1) {
    Fortran::runtime::Terminator terminator{__FILE__, __LINE__};
    terminator.Crash("Too many invalid grid dimensions");
  }
  cudaStream_t defaultStream = 0;
  CUDA_REPORT_IF_ERROR(cudaLaunchCooperativeKernel(kernel, gridDim, blockDim,
      params, smem, stream != nullptr ? (cudaStream_t)*stream : defaultStream));
}

} // extern "C"
