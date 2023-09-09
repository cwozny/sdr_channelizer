#include <Eigen/Dense>
#include <iostream>
#include <cmath>
#include <vector>
#include <Eigen/QR>

void polyfit(const std::vector<double> &t, const std::vector<double> &v, std::vector<double> &coeff, const int order)
{
	// Create Matrix Placeholder of size n x k, n = number of datapoints, k = order of polynomial, for example k = 3 for cubic polynomial
	Eigen::MatrixXd T(t.size(), order + 1);
	Eigen::VectorXd V = Eigen::VectorXd::Map(&v.front(), v.size());
	Eigen::VectorXd result;

	// check to make sure inputs are correct
	assert(t.size() == v.size());
	assert(t.size() >= order + 1);

	// Populate the matrix
	for(size_t i = 0 ; i < t.size(); ++i)
	{
		for(size_t j = 0; j < order + 1; ++j)
		{
			T(i, j) = pow(t.at(i), j);
		}
	}

	// Solve for linear least square fit
	result  = T.householderQr().solve(V);
	coeff.resize(order+1);

	for (int k = 0; k < order+1; k++)
	{
		coeff[k] = result[k];
	}
}

int main(const int argc, const char* argv[])
{
	// toa
	std::vector<double> toa = {-0.0294237136840820,
-0.0294237136840820,
-0.0294234752655029,
-0.0286288261413574,
-0.0286281108856201,
-0.0286281108856201,
-0.0278325080871582,
-0.0270347595214844,
-0.0260267257690430,
-0.0250170230865479,
-0.0240058898925781,
-0.0229933261871338,
-0.0219790935516357,
-0.0209634304046631,
-0.0199460983276367,
-0.0189273357391357,
-0.0179193019866943,
-0.0169112682342529,
-0.0161271095275879,
-0.0153415203094482,
-0.0145545005798340,
-0.0137658119201660,
-0.0129756927490234,
-0.0121839046478271,
-0.0113906860351563,
-0.0105957984924316,
-0.00979948043823242,
-0.00900149345397949,
-0.00799345970153809,
-0.00698375701904297,
-0.00597262382507324,
-0.00496006011962891,
-0.00394582748413086,
-0.00293016433715820,
-0.00191283226013184,
-0.000894069671630859,
0.000113964080810547,
0.00112199783325195,
0.00190615653991699,
0.00269174575805664,
0.00347876548767090,
0.00426745414733887,
0.00505757331848145,
0.00584936141967773,
0.00664281845092773,
0.00743746757507324,
0.00823402404785156,
0.00903177261352539,
0.0100398063659668,
0.0110495090484619,
0.0120606422424316,
0.0130732059478760,
0.0140874385833740,
0.0151033401489258,
0.0161206722259522,
0.0171394348144531,
0.0181474685668945,
0.0191555023193359,
0.0199394226074219,
0.0207250118255615,
0.0215122699737549,
0.0223009586334229,
0.0230910778045654,
0.0238828659057617,
0.0246760845184326,
0.0254709720611572,
0.0262672901153564,
0.0270652770996094,
0.0280733108520508,
0.0290830135345459,
0.0300939083099365,
0.0311067104339600,
0.0321209430694580,
0.0331366062164307,
0.0331375598907471,
0.0331375598907471,
0.0341539382934570,
0.0341541767120361,
0.0341541767120361,
0.0341541767120361,
0.0341544151306152,
0.0341544151306152,
0.0341544151306152,
0.0341544151306152,
0.0341546535491943,
0.0341546535491943};

	// snr
	std::vector<double> snr = {15.0778884318399,
15.1218102908487,
15.1909541037883,
15.4237329181845,
15.3166667947587,
15.1296110544074,
15.9080890441401,
16.3920633395996,
16.9406295544932,
17.4000010554819,
17.8161279397447,
18.1818450657004,
18.5076984107035,
18.8339932592280,
19.1616392835744,
19.4968699321962,
19.8608936354765,
20.2744198253555,
20.6442859592110,
21.0801565387840,
21.4483571840112,
21.7783336949725,
22.1739130769235,
22.5255260871634,
22.8594994721520,
23.1749848442396,
23.5066380945233,
23.8248931390406,
24.0866291693126,
24.4665515965924,
24.7520549508867,
24.8726459534284,
25.1212323867384,
25.2274245757393,
25.3533842223836,
25.4026145100743,
25.4622421721573,
25.5075868912024,
25.4954582162938,
25.5536622534346,
25.5378800348223,
25.4168471257278,
25.4934690669114,
25.4660104377973,
25.3841714893433,
25.3177119824576,
25.2148973703179,
25.0936985638286,
24.9917235181902,
24.8653889144917,
24.5733814837105,
24.1534066832869,
23.9430242397830,
23.4039570247148,
22.9425705296463,
22.4293320281544,
21.8178965508728,
21.1455984612979,
20.5467677853409,
20.1178709480730,
19.3212851261506,
18.7975297491603,
17.9815578842071,
17.5118621464115,
17.1884434311840,
16.9334958519202,
16.7770248298941,
16.6404394629073,
16.5437619048883,
16.5383385841450,
16.5901811464597,
16.1616365647389,
16.0019031194543,
15.3898955697433,
15.0941089050215,
15.0458091828157,
15.1947690039591,
15.1402045349420,
15.2411210667672,
15.1917932878065,
15.0952197506777,
15.0372879129648,
15.0583147970983,
15.0826597047863,
15.0303610875510,
15.0146297604227};

	// placeholder for storing polynomial coefficient
        std::vector<double> p;
	polyfit(toa, snr, p, 2);

	const double a = p[2];
	const double b = p[1];
	const double c = p[0];

	std::cout << "c++: " << p[2] << " " << p[1] << " " << p[0] << std::endl;
	std::cout << "mat: " << -10311.4658792357 << " " << 40.5987322965780 << " " << 24.8206616321435 << std::endl;

	std::cout << -b/(2*a) << std::endl;

	return 0;
}
