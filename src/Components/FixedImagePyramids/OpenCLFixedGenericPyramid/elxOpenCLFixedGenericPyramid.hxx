/*=========================================================================
 *
 *  Copyright UMC Utrecht and contributors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __elxOpenCLFixedGenericPyramid_hxx
#define __elxOpenCLFixedGenericPyramid_hxx

#include "elxOpenCLSupportedImageTypes.h"
#include "elxOpenCLFixedGenericPyramid.h"

// GPU includes
#include "itkGPUImageFactory.h"

// GPU factory includes
#include "itkGPURecursiveGaussianImageFilterFactory.h"
#include "itkGPUCastImageFilterFactory.h"
#include "itkGPUShrinkImageFilterFactory.h"
#include "itkGPUResampleImageFilterFactory.h"
#include "itkGPUIdentityTransformFactory.h"
#include "itkGPULinearInterpolateImageFunctionFactory.h"

namespace elastix
{

/**
 * ******************* Constructor ***********************
 */

template< class TElastix >
OpenCLFixedGenericPyramid< TElastix >
::OpenCLFixedGenericPyramid()
{
  this->m_UseOpenCL = true;

  // Based on the Insight Journal paper:
  // http://insight-journal.org/browse/publication/884
  // it is not beneficial to create pyramids for 2D images with OpenCL.
  // There are also small extra overhead and potential problems may appear.
  // To avoid it, we simply run it on CPU for 2D images.
  if( ImageDimension <= 2 )
  {
    xl::xout[ "warning" ] << "WARNING: Creating the fixed pyramid with OpenCL for 2D images is not beneficial.\n";
    xl::xout[ "warning" ] << "  The OpenCLFixedGenericPyramid is switching back to CPU mode." << std::endl;
    this->m_GPUPyramidCreated = false;
    return;
  }

  // Check if the OpenCL context has been created.
  itk::OpenCLContext::Pointer context = itk::OpenCLContext::GetInstance();
  this->m_ContextCreated = context->IsCreated();
  if( this->m_ContextCreated )
  {
    try
    {
      this->m_GPUPyramid        = GPUPyramidType::New();
      this->m_GPUPyramidCreated = true;
    }
    catch( itk::ExceptionObject & e )
    {
      xl::xout[ "error" ] << "ERROR: Exception during GPU fixed generic pyramid creation: " << e << std::endl;
      this->SwitchingToCPUAndReport( true );
      this->m_GPUPyramidCreated = false;
    }
  }
  else
  {
    this->SwitchingToCPUAndReport( false );
  }
} // end Constructor


/**
 * ******************* BeforeGenerateData ***********************
 */

template< class TElastix >
void
OpenCLFixedGenericPyramid< TElastix >
::BeforeGenerateData( void )
{
  // Local GPU input image
  GPUInputImagePointer gpuInputImage;

  if( this->m_GPUPyramidReady )
  {
    // Create GPU input image
    try
    {
      gpuInputImage = GPUInputImageType::New();
      gpuInputImage->GraftITKImage( this->GetInput() );
      gpuInputImage->AllocateGPU();
      gpuInputImage->GetGPUDataManager()->SetCPUBufferLock( true );
      gpuInputImage->GetGPUDataManager()->SetGPUDirtyFlag( true );
      gpuInputImage->GetGPUDataManager()->UpdateGPUBuffer();
    }
    catch( itk::ExceptionObject & e )
    {
      xl::xout[ "error" ] << "ERROR: Exception during creating GPU input image: " << e << std::endl;
      this->SwitchingToCPUAndReport( true );
    }
  }

  if( this->m_GPUPyramidReady )
  {
    // Set the m_GPUResampler properties the same way as Superclass1
    this->m_GPUPyramid->SetNumberOfLevels( this->GetNumberOfLevels() );
    this->m_GPUPyramid->SetRescaleSchedule( this->GetRescaleSchedule() );
    this->m_GPUPyramid->SetSmoothingSchedule( this->GetSmoothingSchedule() );
    this->m_GPUPyramid->SetUseShrinkImageFilter( this->GetUseShrinkImageFilter() );
    this->m_GPUPyramid->SetComputeOnlyForCurrentLevel( this->GetComputeOnlyForCurrentLevel() );
  }

  if( this->m_GPUPyramidReady )
  {
    try
    {
      this->m_GPUPyramid->SetInput( gpuInputImage );
    }
    catch( itk::ExceptionObject & e )
    {
      xl::xout[ "error" ] << "ERROR: Exception during setting GPU fixed generic pyramid: " << e << std::endl;
      this->SwitchingToCPUAndReport( true );
    }
  }
} // end BeforeGenerateData()


/**
 * ******************* GenerateData ***********************
 */

template< class TElastix >
void
OpenCLFixedGenericPyramid< TElastix >
::GenerateData( void )
{
  if( !this->m_ContextCreated || !this->m_GPUPyramidCreated
    || !this->m_UseOpenCL || !this->m_GPUPyramidReady )
  {
    // Switch to CPU version
    Superclass1::GenerateData();
    return;
  }

  // First execute BeforeGenerateData to configure GPU pyramid
  this->BeforeGenerateData();
  if( !this->m_GPUPyramidReady )
  {
    Superclass1::GenerateData();
    return;
  }

  // Register factories
  this->RegisterFactories();

  // Perform GPU pyramid execution
  this->m_GPUPyramid->Update();

  // Unregister factories
  this->UnregisterFactories();

  // Graft output
  this->GraftOutput( this->m_GPUPyramid->GetOutput() );

} // end GenerateData()


/**
 * ******************* RegisterFactories ***********************
 */

template< class TElastix >
void
OpenCLFixedGenericPyramid< TElastix >
::RegisterFactories( void )
{
  // Typedefs for factories
  typedef itk::GPUImageFactory2< OpenCLImageTypes, OpenCLImageDimentions >
    ImageFactoryType;
  typedef itk::GPURecursiveGaussianImageFilterFactory2< OpenCLImageTypes, OpenCLImageTypes, OpenCLImageDimentions >
    RecursiveGaussianFactoryType;
  typedef itk::GPUCastImageFilterFactory2< OpenCLImageTypes, OpenCLImageTypes, OpenCLImageDimentions >
    CastFactoryType;
  typedef itk::GPUShrinkImageFilterFactory2< OpenCLImageTypes, OpenCLImageTypes, OpenCLImageDimentions >
    ShrinkFactoryType;
  typedef itk::GPUResampleImageFilterFactory2< OpenCLImageTypes, OpenCLImageTypes, OpenCLImageDimentions >
    ResampleFactoryType;
  typedef itk::GPUIdentityTransformFactory2< OpenCLImageDimentions >
    IdentityFactoryType;
  typedef itk::GPULinearInterpolateImageFunctionFactory2< OpenCLImageTypes, OpenCLImageDimentions >
    LinearFactoryType;

  // Create factories
  typename ImageFactoryType::Pointer imageFactory
    = ImageFactoryType::New();
  typename RecursiveGaussianFactoryType::Pointer recursiveFactory
    = RecursiveGaussianFactoryType::New();
  typename CastFactoryType::Pointer castFactory
    = CastFactoryType::New();
  typename ShrinkFactoryType::Pointer shrinkFactory
    = ShrinkFactoryType::New();
  typename ResampleFactoryType::Pointer resampleFactory
    = ResampleFactoryType::New();
  typename IdentityFactoryType::Pointer identityFactory
    = IdentityFactoryType::New();
  typename LinearFactoryType::Pointer linearFactory
    = LinearFactoryType::New();

  // Register factories
  itk::ObjectFactoryBase::RegisterFactory( imageFactory );
  itk::ObjectFactoryBase::RegisterFactory( recursiveFactory );
  itk::ObjectFactoryBase::RegisterFactory( castFactory );
  itk::ObjectFactoryBase::RegisterFactory( shrinkFactory );
  itk::ObjectFactoryBase::RegisterFactory( resampleFactory );
  itk::ObjectFactoryBase::RegisterFactory( identityFactory );
  itk::ObjectFactoryBase::RegisterFactory( linearFactory );

  // Append them
  this->m_Factories.push_back( imageFactory.GetPointer() );
  this->m_Factories.push_back( recursiveFactory.GetPointer() );
  this->m_Factories.push_back( castFactory.GetPointer() );
  this->m_Factories.push_back( shrinkFactory.GetPointer() );
  this->m_Factories.push_back( resampleFactory.GetPointer() );
  this->m_Factories.push_back( identityFactory.GetPointer() );
  this->m_Factories.push_back( linearFactory.GetPointer() );

} // end RegisterFactories()


/**
 * ******************* UnregisterFactories ***********************
 */

template< class TElastix >
void
OpenCLFixedGenericPyramid< TElastix >
::UnregisterFactories( void )
{
  for( std::vector< ObjectFactoryBasePointer >::iterator it = this->m_Factories.begin();
    it != this->m_Factories.end(); ++it )
  {
    itk::ObjectFactoryBase::UnRegisterFactory( *it );
  }
  this->m_Factories.clear();
} // end UnregisterFactories()


/**
 * ******************* BeforeRegistration ***********************
 */

template< class TElastix >
void
OpenCLFixedGenericPyramid< TElastix >
::BeforeRegistration( void )
{
  // Are we using a OpenCL enabled GPU for pyramid?
  this->m_UseOpenCL = true;
  this->m_Configuration->ReadParameter( this->m_UseOpenCL, "OpenCLFixedGenericImagePyramidUseOpenCL", 0 );

} // end BeforeRegistration()


/*
 * ******************* ReadFromFile  ****************************
 */

template< class TElastix >
void
OpenCLFixedGenericPyramid< TElastix >
::ReadFromFile( void )
{
  // OpenCL pyramid specific.
  this->m_UseOpenCL = true;
  this->m_Configuration->ReadParameter( this->m_UseOpenCL, "OpenCLFixedGenericImagePyramidUseOpenCL", 0 );

} // end ReadFromFile()


/**
 * ************************* SwitchingToCPUAndReport ************************
 */

template< class TElastix >
void
OpenCLFixedGenericPyramid< TElastix >
::SwitchingToCPUAndReport( const bool configError )
{
  if( !configError )
  {
    xl::xout[ "warning" ] << "WARNING: The OpenCL context could not be created.\n";
    xl::xout[ "warning" ] << "  The OpenCLFixedGenericImagePyramid is switching back to CPU mode." << std::endl;
  }
  else
  {
    xl::xout[ "warning" ] << "WARNING: Unable to configure the GPU.\n";
    xl::xout[ "warning" ] << "  The OpenCLFixedGenericImagePyramid is switching back to CPU mode." << std::endl;
  }
  this->m_GPUPyramidReady = false;

} // end SwitchingToCPUAndReport()


} // end namespace elastix

#endif // end #ifndef __elxOpenCLFixedGenericPyramid_hxx
