 

#include "TMVA/NeuralNet.h"


namespace TMVA
{
    namespace DNN
    {



        double gaussDouble (double mean, double sigma)
        {
            static std::default_random_engine generator;
            std::normal_distribution<double> distribution (mean, sigma);
            return distribution (generator);
        }


        double uniformDouble (double minValue, double maxValue)
        {
            static std::default_random_engine generator;
            std::uniform_real_distribution<double> distribution(minValue, maxValue);
            return distribution(generator);
        }


    
        int randomInt (int maxValue)
        {
            static std::default_random_engine generator;
            std::uniform_int_distribution<int> distribution(0,maxValue-1);
            return distribution(generator);
        }


        double studenttDouble (double distributionParameter)
        {
            static std::default_random_engine generator;
            std::student_t_distribution<double> distribution (distributionParameter);
            return distribution (generator);
        }


        LayerData::LayerData (size_t inputSize)
            : m_isInputLayer (true)
            , m_hasWeights (false)
            , m_hasGradients (false)
            , m_eModeOutput (ModeOutputValues::DIRECT) 
        {
            m_size = inputSize;
            m_deltas.assign (m_size, 0);
        }



        LayerData::LayerData (const_iterator_type itInputBegin, const_iterator_type itInputEnd, ModeOutputValues eModeOutput)
            : m_isInputLayer (true)
            , m_hasWeights (false)
            , m_hasGradients (false)
            , m_eModeOutput (eModeOutput) 
        {
            m_itInputBegin = itInputBegin;
            m_itInputEnd   = itInputEnd;
            m_size = std::distance (itInputBegin, itInputEnd);
            m_deltas.assign (m_size, 0);
        }




        LayerData::LayerData (size_t _size, 
                              const_iterator_type itWeightBegin, 
                              iterator_type itGradientBegin, 
                              std::shared_ptr<std::function<double(double)>> _activationFunction, 
                              std::shared_ptr<std::function<double(double)>> _inverseActivationFunction,
                              ModeOutputValues eModeOutput)
            : m_size (_size)
            , m_itConstWeightBegin   (itWeightBegin)
            , m_itGradientBegin (itGradientBegin)
            , m_activationFunction (_activationFunction)
            , m_inverseActivationFunction (_inverseActivationFunction)
            , m_isInputLayer (false)
            , m_hasWeights (true)
            , m_hasGradients (true)
            , m_eModeOutput (eModeOutput) 
        {
            m_values.assign (_size, 0);
            m_deltas.assign (_size, 0);
            m_valueGradients.assign (_size, 0);
        }




        LayerData::LayerData (size_t _size, const_iterator_type itWeightBegin, 
                              std::shared_ptr<std::function<double(double)>> _activationFunction, 
                              ModeOutputValues eModeOutput)
            : m_size (_size)
            , m_itConstWeightBegin   (itWeightBegin)
            , m_activationFunction (_activationFunction)
            , m_isInputLayer (false)
            , m_hasWeights (true)
            , m_hasGradients (false)
            , m_eModeOutput (eModeOutput) 
        {
            m_values.assign (_size, 0);
        }



        typename LayerData::container_type LayerData::computeProbabilities ()
        {
            container_type probabilitiesContainer;
            if (TMVA::DNN::isFlagSet (ModeOutputValues::SIGMOID, m_eModeOutput))
            {
                std::transform (begin (m_values), end (m_values), std::back_inserter (probabilitiesContainer), (*Sigmoid.get ()));
            }
            else if (TMVA::DNN::isFlagSet (ModeOutputValues::SOFTMAX, m_eModeOutput))
            {
                double sum = 0;
                probabilitiesContainer = m_values;
                std::for_each (begin (probabilitiesContainer), end (probabilitiesContainer), [&sum](double& p){ p = std::exp (p); sum += p; });
                if (sum != 0)
                    std::for_each (begin (probabilitiesContainer), end (probabilitiesContainer), [sum ](double& p){ p /= sum; });
            }
            else
            {
                probabilitiesContainer.assign (begin (m_values), end (m_values));
            }
            return probabilitiesContainer;
        }





        Layer::Layer (size_t _numNodes, EnumFunction _activationFunction, ModeOutputValues eModeOutputValues) 
            : m_numNodes (_numNodes) 
            , m_eModeOutputValues (eModeOutputValues)
            , m_activationFunctionType (_activationFunction)
        {
            for (size_t iNode = 0; iNode < _numNodes; ++iNode)
            {
                auto actFnc = Linear;
                auto invActFnc = InvLinear;
                switch (_activationFunction)
                {
                case EnumFunction::ZERO:
                    actFnc = ZeroFnc;
                    invActFnc = ZeroFnc;
                    break;
                case EnumFunction::LINEAR:
                    actFnc = Linear;
                    invActFnc = InvLinear;
                    break;
                case EnumFunction::TANH:
                    actFnc = Tanh;
                    invActFnc = InvTanh;
                    break;
                case EnumFunction::RELU:
                    actFnc = ReLU;
                    invActFnc = InvReLU;
                    break;
                case EnumFunction::SYMMRELU:
                    actFnc = SymmReLU;
                    invActFnc = InvSymmReLU;
                    break;
                case EnumFunction::TANHSHIFT:
                    actFnc = TanhShift;
                    invActFnc = InvTanhShift;
                    break;
                case EnumFunction::SOFTSIGN:
                    actFnc = SoftSign;
                    invActFnc = InvSoftSign;
                    break;
                case EnumFunction::SIGMOID:
                    actFnc = Sigmoid;
                    invActFnc = InvSigmoid;
                    break;
                case EnumFunction::GAUSS:
                    actFnc = Gauss;
                    invActFnc = InvGauss;
                    break;
                case EnumFunction::GAUSSCOMPLEMENT:
                    actFnc = GaussComplement;
                    invActFnc = InvGaussComplement;
                    break;
                }
                m_activationFunction = actFnc;
                m_inverseActivationFunction = invActFnc;
            }
        }










        Settings::Settings (TString name,
                            size_t _convergenceSteps, size_t _batchSize, size_t _testRepetitions, 
                            double _factorWeightDecay, EnumRegularization eRegularization,
                            MinimizerType _eMinimizerType, double _learningRate, 
                            double _momentum, int _repetitions, bool _useMultithreading, 
                            bool _doBatchNormalization)
            : m_timer (100, name)
            , m_minProgress (0)
            , m_maxProgress (100)
            , m_convergenceSteps (_convergenceSteps)
            , m_batchSize (_batchSize)
            , m_testRepetitions (_testRepetitions)
            , m_factorWeightDecay (_factorWeightDecay)
            , count_E (0)
            , count_dE (0)
            , count_mb_E (0)
            , count_mb_dE (0)
            , m_regularization (eRegularization)
            , fLearningRate (_learningRate)
            , fMomentum (_momentum)
            , fRepetitions (_repetitions)
            , fMinimizerType (_eMinimizerType)
            , m_convergenceCount (0)
            , m_maxConvergenceCount (0)
            , m_minError (1e10)
            , m_useMultithreading (_useMultithreading)
            , m_doBatchNormalization (_doBatchNormalization)
            , fMonitoring (NULL)
        {
        }
    
        Settings::~Settings () 
        {
        }














/** \brief action to be done when the training cycle is started (e.g. update some monitoring output)
 *
 */
        void ClassificationSettings::startTrainCycle () 
        {
            if (fMonitoring)
            {
                create ("ROC", 100, 0, 1, 100, 0, 1);
                create ("Significance", 100, 0, 1, 100, 0, 3);
                create ("OutputSig", 100, 0, 1);
                create ("OutputBkg", 100, 0, 1);
                fMonitoring->ProcessEvents ();
            }
        }

/** \brief action to be done when the training cycle is ended (e.g. update some monitoring output)
 *
 */
        void ClassificationSettings::endTrainCycle (double /*error*/) 
        {
            if (fMonitoring) fMonitoring->ProcessEvents ();
        }

/** \brief action to be done after the computation of a test sample (e.g. update some monitoring output)
 *
 */
    void ClassificationSettings::testSample (double /*error*/, double output, double target, double weight)
        {
            
            m_output.push_back (output);
            m_targets.push_back (target);
            m_weights.push_back (weight);
        }


/** \brief action to be done when the test cycle is started (e.g. update some monitoring output)
 *
 */
        void ClassificationSettings::startTestCycle () 
        {
            m_output.clear ();
            m_targets.clear ();
            m_weights.clear ();
        }

/** \brief action to be done when the training cycle is ended (e.g. update some monitoring output)
 *
 */
        void ClassificationSettings::endTestCycle () 
        {
            if (m_output.empty ())
                return;
            double minVal = *std::min_element (begin (m_output), end (m_output));
            double maxVal = *std::max_element (begin (m_output), end (m_output));
            const size_t numBinsROC = 1000;
            const size_t numBinsData = 100;

            std::vector<double> truePositives (numBinsROC+1, 0);
            std::vector<double> falsePositives (numBinsROC+1, 0);
            std::vector<double> trueNegatives (numBinsROC+1, 0);
            std::vector<double> falseNegatives (numBinsROC+1, 0);

            std::vector<double> x (numBinsData, 0);
            std::vector<double> datSig (numBinsData+1, 0);
            std::vector<double> datBkg (numBinsData+1, 0);

            double binSizeROC = (maxVal - minVal)/(double)numBinsROC;
            double binSizeData = (maxVal - minVal)/(double)numBinsData;

            double sumWeightsSig = 0.0;
            double sumWeightsBkg = 0.0;

            for (size_t b = 0; b < numBinsData; ++b)
            {
                double binData = minVal + b*binSizeData;
                x.at (b) = binData;
            }

            if (fabs(binSizeROC) < 0.0001)
                return;

            for (size_t i = 0, iEnd = m_output.size (); i < iEnd; ++i)
            {
                double val = m_output.at (i);
                double truth = m_targets.at (i);
                double weight = m_weights.at (i);

                bool isSignal = (truth > 0.5 ? true : false);

                if (m_sumOfSigWeights != 0 && m_sumOfBkgWeights != 0)
                {
                    if (isSignal)
                        weight *= m_sumOfSigWeights;
                    else
                        weight *= m_sumOfBkgWeights;
                }

                size_t binROC = (val-minVal)/binSizeROC;
                size_t binData = (val-minVal)/binSizeData;

                if (isSignal)
                {
                    for (size_t n = 0; n <= binROC; ++n)
                    {
                        truePositives.at (n) += weight;
                    }
                    for (size_t n = binROC+1; n < numBinsROC; ++n)
                    {
                        falseNegatives.at (n) += weight;
                    }

                    datSig.at (binData) += weight;
                    sumWeightsSig += weight;
                }
                else
                {
                    for (size_t n = 0; n <= binROC; ++n)
                    {
                        falsePositives.at (n) += weight;
                    }
                    for (size_t n = binROC+1; n < numBinsROC; ++n)
                    {
                        trueNegatives.at (n) += weight;
                    }

                    datBkg.at (binData) += weight;
                    sumWeightsBkg += weight;
                }
            }

            std::vector<double> sigEff;
            std::vector<double> backRej;

            double bestSignificance = 0;
            double bestCutSignificance = 0;

            double numEventsScaleFactor = 1.0;
            if (m_scaleToNumEvents > 0)
            {
                size_t numEvents = m_output.size ();
                numEventsScaleFactor = double (m_scaleToNumEvents)/double (numEvents);
            }

            clear ("ROC");
            clear ("Significance");

            for (size_t i = 0; i < numBinsROC; ++i)
            {
                double tp = truePositives.at (i) * numEventsScaleFactor;
                double fp = falsePositives.at (i) * numEventsScaleFactor;
                double tn = trueNegatives.at (i) * numEventsScaleFactor;
                double fn = falseNegatives.at (i) * numEventsScaleFactor;

                double seff = (tp+fn == 0.0 ? 1.0 : (tp / (tp+fn)));
                double brej = (tn+fp == 0.0 ? 0.0 : (tn / (tn+fp)));

                sigEff.push_back (seff);
                backRej.push_back (brej);
            
//            m_histROC->Fill (seff, brej);
                addPoint ("ROC", seff, brej); // x, y


                double currentCut = (i * binSizeROC)+minVal;

                double sig = tp;
                double bkg = fp;
                double significance = sig / sqrt (sig + bkg);
                if (significance > bestSignificance)
                {
                    bestSignificance = significance;
                    bestCutSignificance = currentCut;
                }

                addPoint ("Significance", currentCut, significance);
//            m_histSignificance->Fill (currentCut, significance);
            }

            m_significances.push_back (bestSignificance);
            static size_t testCycle = 0;

            clear ("OutputSig");
            clear ("OutputBkg");
            for (size_t i = 0; i < numBinsData; ++i)
            {
                addPoint ("OutputSig", x.at (i), datSig.at (i)/sumWeightsSig);
                addPoint ("OutputBkg", x.at (i), datBkg.at (i)/sumWeightsBkg);
                // m_histOutputSignal->Fill (x.at (i), datSig.at (1)/sumWeightsSig);
                // m_histOutputBackground->Fill (x.at (i), datBkg.at (1)/sumWeightsBkg);
            }

       
            ++testCycle;

            if (fMonitoring)
            {
                plot ("ROC", "", 2, kRed);
                plot ("Significance", "", 3, kRed);
                plot ("OutputSig", "", 4, kRed);
                plot ("OutputBkg", "same", 4, kBlue);
                fMonitoring->ProcessEvents ();
            }

            m_cutValue = bestCutSignificance;
        }


/** \brief check for convergence 
 *
 */
        bool Settings::hasConverged (double testError)
        {
            // std::cout << "check convergence; minError " << m_minError << "  current " << testError
            //           << "  current convergence count " << m_convergenceCount << std::endl;
            if (testError < m_minError*0.999)
            {
                m_convergenceCount = 0;
                m_minError = testError;
            }
            else
            {
                ++m_convergenceCount;
                m_maxConvergenceCount = std::max (m_convergenceCount, m_maxConvergenceCount);
            }


            if (m_convergenceCount >= convergenceSteps () || testError <= 0)
                return true;

            return false;
        }



/** \brief set the weight sums to be scaled to (preparations for monitoring output)
 *
 */
        void ClassificationSettings::setWeightSums (double sumOfSigWeights, double sumOfBkgWeights)
        {
            m_sumOfSigWeights = sumOfSigWeights; m_sumOfBkgWeights = sumOfBkgWeights;
        }
    
/** \brief preparation for monitoring output
 *
 */
        void ClassificationSettings::setResultComputation (
            std::string _fileNameNetConfig,
            std::string _fileNameResult,
            std::vector<Pattern>* _resultPatternContainer)
        {
            m_pResultPatternContainer = _resultPatternContainer;
            m_fileNameResult = _fileNameResult;
            m_fileNameNetConfig = _fileNameNetConfig;
        }






    

/** \brief compute the number of weights given the size of the input layer
 *
 */
        size_t Net::numWeights (size_t trainingStartLayer) const 
        {
            size_t num (0);
            size_t index (0);
            size_t prevNodes (inputSize ());
            for (auto& layer : m_layers)
            {
                if (index >= trainingStartLayer)
                    num += layer.numWeights (prevNodes);
                prevNodes = layer.numNodes ();
                ++index;
            }
            return num;
        }



/** \brief prepare the drop-out container given the provided drop-fractions
 *
 */
        void Net::fillDropContainer (DropContainer& dropContainer, double dropFraction, size_t numNodes) const
        {
            size_t numDrops = dropFraction * numNodes;
            if (numDrops >= numNodes) // maintain at least one node
                numDrops = numNodes - 1;
            dropContainer.insert (end (dropContainer), numNodes-numDrops, true); // add the markers for the nodes which are enabled
            dropContainer.insert (end (dropContainer), numDrops, false); // add the markers for the disabled nodes
            // shuffle 
            std::random_shuffle (end (dropContainer)-numNodes, end (dropContainer)); // shuffle enabled and disabled markers
        }







 
    }; // namespace DNN
}; // namespace TMVA
