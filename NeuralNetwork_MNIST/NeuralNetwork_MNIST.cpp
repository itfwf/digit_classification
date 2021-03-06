﻿// NeuralNetwork_MNIST.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include<random>
#include "MNIST.h"
using namespace std;



struct Connection
{
	float weight;
	float deltaWeight;
};

class Neuron;

typedef vector<Neuron> Layer;

class Neuron
{
public:
	Neuron(unsigned numOutputs, unsigned myIndex);
	void saveWeights(const Layer &prevLayer);
	void setOutputVal(float val) { m_outputVal = val; }
	float getOutputVal(void) const { return m_outputVal; }
	void feedForward(const Layer &prevLayer);
	void calcOutputGradients(float targetVals);
	void calcHiddenGradients(const Layer &nextLayer);
	void updateInputWeights(Layer &prevLayer);
private:
	static float eta; //  overall net training rate
	static float alpha; 
	static float transferFunction(float x);
	static float transferFunctionDerivative(float x);
	// randomWeight: -0.1 - 0.1
	static float randomWeight(void) {
		static std::default_random_engine generator;
		std::uniform_real_distribution<float> distribution(-0.01, 0.01);
		return distribution(generator);
	
	}
	float sumDOW(const Layer &nextLayer) const;
	float m_outputVal;
	vector<Connection> m_outputWeights;
	unsigned m_myIndex;
	float m_gradient;
};

float Neuron::eta = 0.15; // overall net learning rate
float Neuron::alpha = 0.5; // momentum, multiplier of last deltaWeight, [0.0..n]


void Neuron::updateInputWeights(Layer &prevLayer)
{


	for (unsigned n = 0; n < prevLayer.size(); ++n)
	{
		Neuron &neuron = prevLayer[n];
		float oldDeltaWeight = neuron.m_outputWeights[m_myIndex].deltaWeight;

		float newDeltaWeight =

			eta
			* neuron.getOutputVal()
			* m_gradient

			+ alpha
			* oldDeltaWeight;
		neuron.m_outputWeights[m_myIndex].deltaWeight = newDeltaWeight;
		neuron.m_outputWeights[m_myIndex].weight += newDeltaWeight;
	}
}
float Neuron::sumDOW(const Layer &nextLayer) const
{
	float sum = 0.0;

	// Sum our contributions of the errors at the nodes we feed

	for (unsigned n = 0; n < nextLayer.size() - 1; ++n)
	{
		sum += m_outputWeights[n].weight * nextLayer[n].m_gradient;
	}

	return sum;
}

void Neuron::calcHiddenGradients(const Layer &nextLayer)
{
	float dow = sumDOW(nextLayer);
	m_gradient = dow * Neuron::transferFunctionDerivative(m_outputVal);
}
void Neuron::calcOutputGradients(float targetVals)
{
	float delta = targetVals - m_outputVal;
	m_gradient = delta * Neuron::transferFunctionDerivative(m_outputVal);
}

float Neuron::transferFunction(float x)
{
	// tanh - output range [-1.0..1.0]
	//return tanh(x);
	return 1 / (1 + exp(-x));
}

float Neuron::transferFunctionDerivative(float x)
{
	// tanh derivative
	//return 1.0 - x * x;
	return x * (1 - x);
}

void Neuron::feedForward(const Layer &prevLayer)
{
	float sum = 0.0;

	// Sum the previous layer's outputs (which are our inputs)
	// Include the bias node from the previous layer.

	for (unsigned n = 0; n < prevLayer.size(); ++n)
	{
		sum += prevLayer[n].getOutputVal() *
			prevLayer[n].m_outputWeights[m_myIndex].weight;
	}

	m_outputVal = Neuron::transferFunction(sum);
}

Neuron::Neuron(unsigned numOutputs, unsigned myIndex)
{	
	ofstream of("wee.txt", std::ios_base::app);
	for (unsigned c = 0; c < numOutputs; ++c) {
		m_outputWeights.push_back(Connection());
		m_outputWeights.back().weight = randomWeight();
		of << m_outputWeights[c].weight << "\n";
	}

	m_myIndex = myIndex;
}

void Neuron::saveWeights(const Layer &prevLayer) {
	ofstream of("we.txt", ios_base::app);
	for (unsigned n = 0; n < prevLayer.size(); ++n)
	{
		of << prevLayer[n].m_outputWeights[m_myIndex].weight << "\n";
	}

}

class Net
{
public:
	Net(const vector<unsigned> &topology);
	void feedForward(const vector<float> &inputVals);
	void backProp(const vector<float> &targetVals);
	void saveWeights();

	void getResults(vector<float> &resultVals) const;
	float getRecentAverageError(void) const { return m_recentAverageError; }

private:
	vector<Layer> m_layers; 
	float m_error;
	float m_recentAverageError;
	static float m_recentAverageSmoothingFactor;
};

float Net::m_recentAverageSmoothingFactor = 100.0; 

void Net::getResults(vector<float> &resultVals) const
{
	resultVals.clear();

	for (unsigned n = 0; n < m_layers.back().size() - 1; ++n)
	{
		resultVals.push_back(m_layers.back()[n].getOutputVal());
	}
}

void Net::backProp(const std::vector<float> &targetVals)
{
	// Calculate overal net error (RMS of output neuron errors)

	Layer &outputLayer = m_layers.back();
	m_error = 0.0;

	for (unsigned n = 0; n < outputLayer.size() - 1; ++n)
	{
		float delta = targetVals[n] - outputLayer[n].getOutputVal();
		m_error += delta * delta;
	}
	m_error /= outputLayer.size() - 1; // get average error squared
	m_error = sqrt(m_error); // RMS

	// Implement a recent average measurement:

	m_recentAverageError =
		(m_recentAverageError * m_recentAverageSmoothingFactor + m_error)
		/ (m_recentAverageSmoothingFactor + 1.0);
	// Calculate output layer gradients

	for (unsigned n = 0; n < outputLayer.size() - 1; ++n)
	{
		outputLayer[n].calcOutputGradients(targetVals[n]);
	}
	// Calculate gradients on hidden layers

	for (unsigned layerNum = m_layers.size() - 2; layerNum > 0; --layerNum)
	{
		Layer &hiddenLayer = m_layers[layerNum];
		Layer &nextLayer = m_layers[layerNum + 1];

		for (unsigned n = 0; n < hiddenLayer.size(); ++n)
		{
			hiddenLayer[n].calcHiddenGradients(nextLayer);
		}
	}

	// For all layers from outputs to first hidden layer,
	// update connection weights

	for (unsigned layerNum = m_layers.size() - 1; layerNum > 0; --layerNum)
	{
		Layer &layer = m_layers[layerNum];
		Layer &prevLayer = m_layers[layerNum - 1];

		for (unsigned n = 0; n < layer.size() - 1; ++n)
		{
			layer[n].updateInputWeights(prevLayer);
		}
	}
}
void Net::saveWeights() {

	// Forward propagate
	for (unsigned layerNum = 1; layerNum < m_layers.size(); ++layerNum) {
		Layer &prevLayer = m_layers[layerNum - 1];
		for (unsigned n = 0; n < m_layers[layerNum].size() - 1; ++n) {
			m_layers[layerNum][n].saveWeights(prevLayer);
		}
	}

}

void Net::feedForward(const vector<float> &inputVals)
{
	// Check the num of inputVals euqal to neuronnum expect bias
	assert(inputVals.size() == m_layers[0].size() - 1);

	// Assign {latch} the input values into the input neurons
	for (unsigned i = 0; i < inputVals.size(); ++i) {
		m_layers[0][i].setOutputVal(inputVals[i]);
	}

	// Forward propagate
	for (unsigned layerNum = 1; layerNum < m_layers.size(); ++layerNum) {
		Layer &prevLayer = m_layers[layerNum - 1];
		for (unsigned n = 0; n < m_layers[layerNum].size() - 1; ++n) {
			m_layers[layerNum][n].feedForward(prevLayer);
		}
	}
}
Net::Net(const vector<unsigned> &topology)
{
	unsigned numLayers = topology.size();
	for (unsigned layerNum = 0; layerNum < numLayers; ++layerNum) {
		m_layers.push_back(Layer());
		// numOutputs of layer[i] is the numInputs of layer[i+1]
		// numOutputs of last layer is 0
		unsigned numOutputs = layerNum == topology.size() - 1 ? 0 : topology[layerNum + 1];

		// We have made a new Layer, now fill it ith neurons, and
		// add a bias neuron to the layer:
		for (unsigned neuronNum = 0; neuronNum <= topology[layerNum]; ++neuronNum) {
			m_layers.back().push_back(Neuron(numOutputs, neuronNum));
			//cout << "Mad a Neuron!" << endl;
		}

		// Force the bias node's output value to 1.0. It's the last neuron created above
		m_layers.back().back().setOutputVal(1.0);
	}
}

void showVectorVals(ofstream &of,string label, vector<float> &v)
{
	of << label << " ";
	for (unsigned i = 0; i < v.size(); ++i)
	{
		of  << v[i] << " ";
	}
	
}
int main()
{
	MNIST mnist("D:\\");
	//cout << mnist.trainingData[0].label;
	//e.g., {3, 2, 1 }
	vector<unsigned> topology = { 784, 65,10 };

	Net myNet(topology);
	

	vector<float> inputVals, targetVals, resultVals;
	int trainingPass = 0;
	ofstream out("out.txt");
	while (trainingPass<60000)
	{
		
		//showVectorVals(": Inputs :", mnist.testData[trainingPass].pixelData);
		myNet.feedForward(mnist.trainingData[trainingPass].pixelData);
		
		myNet.getResults(resultVals);

		//assert(targetVals.size() == topology.back());

		myNet.backProp(mnist.trainingData[trainingPass].output);
		out << "\nLabel: " << mnist.trainingData[trainingPass].label;
		showVectorVals(out,"\nOut :", resultVals);
		targetVals = mnist.trainingData[trainingPass].output;
		showVectorVals(out, "\nTarget : ", targetVals);
		//cout << "Net recent average error: "<< myNet.getRecentAverageError() << endl;
		trainingPass++;
	}
	myNet.saveWeights();
	out.close();

	ofstream of("test.txt");
	for (int i = 0; i < 10000; i++) {
		myNet.feedForward(mnist.testData[i].pixelData);
		myNet.getResults(resultVals);
		of << "\nLabel : " << mnist.testData[i].label;
		showVectorVals(of, "\nResult: ", resultVals);
	}
	cout << endl << "Done" << endl;

}
