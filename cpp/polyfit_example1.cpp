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
	std::vector<double> toa = {0.00754761695861816,
					0.00833463668823242,
					0.00833463668823242,
					0.00833487510681152,
					0.00833487510681152,
					0.00833487510681152,
					0.00833487510681152,
					0.00912332534790039,
					0.00991344451904297,
					0.0107052326202393,
					0.0114984512329102,
					0.0122933387756348,
					0.0130896568298340,
					0.0138876438140869,
					0.0148956775665283,
					0.0159051418304443,
					0.0169162750244141,
					0.0179290771484375,
					0.0189433097839355,
					0.0199589729309082,
					0.0209763050079346,
					0.0219950675964355,
					0.0230031013488770,
					0.0240111351013184,
					0.0247950553894043,
					0.0255806446075439,
					0.0263679027557373,
					0.0271565914154053,
					0.0279467105865479,
					0.0287384986877441,
					0.0295317173004150,
					0.0303266048431397,
					0.0311229228973389,
					0.0319209098815918,
					0.0329289436340332,
					0.0339384078979492,
					0.0349495410919189,
					0.0359623432159424,
					0.0369765758514404,
					0.0379922389984131,
					0.0390095710754395,
					0.0400283336639404,
					0.0410363674163818,
					0.0420444011688232,
					0.0428285598754883,
					0.0436141490936279,
					0.0444011688232422,
					0.0451898574829102,
					0.0459799766540527,
					0.0467717647552490,
					0.0475649833679199,
					0.0483598709106445,
					0.0491561889648438,
					0.0499541759490967,
					0.0509622097015381,
					0.0519719123840332,
					0.0529830455780029,
					0.0539956092834473,
					0.0550098419189453,
					0.0560255050659180,
					0.0570428371429443,
					0.0580618381500244,
					0.0590698719024658,
					0.0600779056549072,
					0.0608618259429932,
					0.0616474151611328,
					0.0624344348907471,
					0.0632231235504150,
					0.0640134811401367,
					0.0648052692413330,
					0.0655984878540039,
					0.0663933753967285,
					0.0671896934509277,
					0.0679876804351807,
					0.0689957141876221,
					0.0700051784515381,
					0.0710163116455078,
					0.0720291137695313,
					0.0730433464050293,
					0.0740592479705811,
					0.0740592479705811,
					0.0740594863891602,
					0.0740597248077393,
					0.0740597248077393,
					0.0740597248077393,
					0.0740597248077393,
					0.0740597248077393};

	// snr
	std::vector<double> snr = {15.1026567937532,
					15.0661639605975,
					15.1154388349772,
					15.0328436021563,
					15.0737784146573,
					15.2532905525902,
					15.0641017928628,
					15.4418796701464,
					15.7035499230186,
					16.1909645690640,
					16.8064620081082,
					17.2690698266698,
					17.6779725386105,
					17.9900824413228,
					18.2800051527463,
					18.4489463703231,
					18.7627788206378,
					19.3068174673998,
					19.2170079030160,
					19.7839318740749,
					19.8019310171567,
					20.0237612166299,
					20.3233165760581,
					20.7358608429518,
					21.1101652107328,
					21.6843526343457,
					21.8599146367351,
					22.1983003768851,
					22.5577629191693,
					23.1195724842348,
					23.2045600771205,
					23.4441945002147,
					23.7492572199804,
					24.0123785310474,
					24.4246365343960,
					24.8126243632106,
					25.0200969871791,
					24.9361694545998,
					25.3789533915855,
					25.3000551696970,
					25.4779819500411,
					25.6237955604526,
					25.6544174002431,
					25.6759925187585,
					25.4383084671101,
					25.6474622689303,
					25.0230887356941,
					25.3525410957466,
					25.0611848578664,
					24.9911875273578,
					25.1340148023044,
					25.1421107107246,
					25.0345814474490,
					24.8833656979189,
					24.6079661511751,
					24.2281848629641,
					24.0755719030422,
					23.9361788000060,
					23.3340199602253,
					23.0400579695784,
					22.7016518231935,
					22.0385556646672,
					21.5019559504465,
					20.9252047561541,
					20.2677725973181,
					19.5146889172296,
					19.1119497758608,
					18.3240487081606,
					17.9655480280179,
					17.3629799288555,
					16.8781888620079,
					16.5747366355757,
					16.4631276096699,
					16.4781776065837,
					16.4887371062103,
					16.3837041410019,
					16.0183637230599,
					15.8700226537264,
					15.4045065174409,
					15.0314023430414,
					15.1847361146867,
					15.1953247840263,
					15.2194038180862,
					15.0159402590543,
					15.1151093622444,
					15.0389948441611,
					15.0307893098423};

	// placeholder for storing polynomial coefficient
        std::vector<double> p;
	polyfit(toa, snr, p, 2);

	const double a = p[2];
	const double b = p[1];
	const double c = p[0];

	std::cout << "c++: " << p[2] << " " << p[1] << " " << p[0] << std::endl;
	std::cout << "mat: " << -9544.85820080718 << " " << 780.495030719760 << " " << 8.81907904918576 << std::endl;

	std::cout << -b/(2*a) << std::endl;

	return 0;
}
