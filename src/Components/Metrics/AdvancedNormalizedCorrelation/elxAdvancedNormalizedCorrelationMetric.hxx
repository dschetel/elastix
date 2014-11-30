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
#ifndef __elxAdvancedNormalizedCorrelationMetric_HXX__
#define __elxAdvancedNormalizedCorrelationMetric_HXX__

#include "elxAdvancedNormalizedCorrelationMetric.h"
#include "itkTimeProbe.h"


namespace elastix
{

/**
 * ***************** BeforeEachResolution ***********************
 */

template< class TElastix >
void
AdvancedNormalizedCorrelationMetric< TElastix >
::BeforeEachResolution( void )
{
  /** Get the current resolution level. */
  unsigned int level
    = ( this->m_Registration->GetAsITKBaseType() )->GetCurrentLevel();

  /** Get and set SubtractMean. Default true. */
  bool subtractMean = true;
  this->GetConfiguration()->ReadParameter( subtractMean, "SubtractMean",
    this->GetComponentLabel(), level, 0 );
  this->SetSubtractMean( subtractMean );

  /** Set moving image derivative scales. */
  this->SetUseMovingImageDerivativeScales( false );
  MovingImageDerivativeScalesType movingImageDerivativeScales;
  movingImageDerivativeScales.Fill( 1.0 );
  bool usescales = true;
  for( unsigned int i = 0; i < MovingImageDimension; ++i )
  {
    usescales &= this->GetConfiguration()->ReadParameter(
      movingImageDerivativeScales[ i ], "MovingImageDerivativeScales",
      this->GetComponentLabel(), i, -1, false );
  }
  if( usescales )
  {
    this->SetUseMovingImageDerivativeScales( true );
    this->SetMovingImageDerivativeScales( movingImageDerivativeScales );
    elxout << "Multiplying moving image derivatives by: "
           << movingImageDerivativeScales << std::endl;
  }

} // end BeforeEachResolution()


/**
 * ******************* Initialize ***********************
 */

template< class TElastix >
void
AdvancedNormalizedCorrelationMetric< TElastix >
::Initialize( void ) throw ( itk::ExceptionObject )
{
  itk::TimeProbe timer;
  timer.Start();
  this->Superclass1::Initialize();
  timer.Stop();
  elxout << "Initialization of AdvancedNormalizedCorrelation metric took: "
    << static_cast< long >( timer.GetMean() * 1000 ) << " ms." << std::endl;

} // end Initialize()


} // end namespace elastix

#endif // end #ifndef __elxAdvancedNormalizedCorrelationMetric_HXX__
