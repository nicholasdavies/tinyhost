#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include "Config/config.h"
#include "Randomizer/randomizer.h"
using namespace std;

Parameters P;
Randomizer R;

void Check(vector<double>& param, int size, string name) // Check parameter is correct size
{
    if (param.size() != size)
        throw runtime_error("Incorrect size for parameter " + name);
}

void Normalize(double* x)   // Normalize carriage
{
    double total = accumulate(x, x + P.n_strains, 0.0, [](double X, double x) { return X + (x > 0 ? x : 0); });
    if (total > 0)
        for (int s = 0; s < P.n_strains; ++s)
            if (x[s] > 0)
                x[s] /= total;
}

int main(int argc, char* argv[])
{
    const int Transmission = 0, Clearance = 0x10000, Treatment = 0x20000, Birth = 0x30000, Transfer = 0x40000;
    int run = 0; string filename = "\n"; ofstream fout; ostringstream sout;

    // Iterate over parameter sets
    for (P.Read(argc, argv); P.Good(); P.NextSweep(), ++run)
    {
        Check(P.w,     P.n_strains,     "w");
        Check(P.beta,  P.n_strains,     "beta");
        Check(P.theta, P.n_strains,     "theta");
        Check(P.u,     P.n_strains / 2, "u");

        vector<double> X(P.n_hosts * P.n_strains, 0.0), ww(P.n_strains), l(P.n_strains);    // Carriage matrix, per-time-step growth rates, population-level carriage
        vector<int> events;                                                                 // Event storage

        P.Write(cout);  // Print parameters
        for (int s = 0; s < P.n_strains; ++s) // Calculate per-time-step growth rates
            ww[s] = pow(P.w[s], P.t_step);
        for (int i = 0, n = R.Poisson(P.n_hosts * P.init); i < n; ++i) // Inoculate hosts with a random strain at rate P.init
            X[P.n_strains * i + R.Discrete(P.n_strains)] = 1;

        // Iterate over each time step
        for (int g = 0; g < P.t_max / P.t_step + 0.5; ++g)
        {
            // 1. Calculate force of infection for each strain and update hosts
            fill(l.begin(), l.end(), 0.0);
            for (int i = 0; i < P.n_strains * P.n_hosts; i += P.n_strains)
            {
                double total = 0;   // Enforce host minimum carriage, grow strains, and tally host carriage
                for (int s = 0; s < P.n_strains; ++s)
                    if (X[i + s] > 0)
                        total += X[i + s] = (X[i + s] < P.min_carriage ? 0 : X[i + s] * ww[s]);

                if (total > 0)      // Enforce host carrying capacity and tally population carriage
                    for (int s = 0; s < P.n_strains; ++s)
                        if (X[i + s] > 0)
                            l[s] += X[i + s] /= total;
            }
            for (auto& ll : l)      // Calculate effective population carriage
                ll = max(ll, P.min_carriers) / P.n_hosts;

            // 2. Choose events and randomize their order
            events.clear();
            for (int s = 0; s < P.n_strains; ++s)   events.insert(events.end(), R.Poisson(P.n_hosts * P.beta[s] * l[s] * P.t_step), Transmission | s);
            for (int t = 0; t < P.n_strains/2; ++t) events.insert(events.end(), R.Poisson(P.n_hosts * P.u[t] * P.t_step), Clearance | t);
            events.insert(events.end(), R.Poisson(P.n_hosts * P.tau * P.t_step), Treatment);
            events.insert(events.end(), R.Poisson(P.n_hosts * P.birth_rate * P.t_step), Birth);
            events.insert(events.end(), R.Poisson(P.n_hosts * P.gamma * P.t_step), Transfer);
            R.Shuffle(events.begin(), events.end());

            // 3. Execute events
            for (auto e : events)
            {
                bool normalize = false;
                int j = e & 0xFFFF;
                double* x = X.data() + P.n_strains * R.Discrete(P.n_hosts);
                switch (e & 0xF0000)
                {
                    case Transmission:  // Colonise host with strain j
                        if (P.k == 1 || all_of(x, x + P.n_strains, [](double y) { return y <= 0; }) || R.Bernoulli(P.k)) // If there is no blocking...
                            if (!P.immunity || x[j] >= 0 || R.Bernoulli(1 + x[j])) // and no immunity...
                                { x[j] = max(x[j], 0.0) + P.iota; Normalize(x); }
                        break;

                    case Clearance:     // Clear serotype j from host, possibly bringing other serotypes with it
                        if (x[j * 2] > 0 || x[j * 2 + 1] > 0)
                        {
                            x[j * 2] = x[j * 2 + 1] = -P.sigma;
                            if (R.Bernoulli(P.v))
                                for (int s = 0; s < P.n_strains; ++s)
                                    if (x[s] > 0) x[s] = 0;
                            Normalize(x);
                        }
                        break;
                        
                    case Treatment:     // Eliminate all sensitive strains from host
                        for (int s = 0; s < P.n_strains; s += 2)
                            if (x[s] > 0)
                                { x[s] = 0; normalize = true; }
                        if (normalize)
                            Normalize(x);
                        break;

                    case Birth:         // Replace host with new, naive host
                        fill(x, x + P.n_strains, 0.0);
                        break;

                    case Transfer:      // Colonise host with strains carried by a random host
                        if (P.k == 1 || all_of(x, x + P.n_strains, [](double y) { return y <= 0; }) || R.Bernoulli(P.k)) // If there is no blocking...
                        {
                            double* xx = X.data() + P.n_strains * R.Discrete(P.n_hosts); // Choose contacted host
                            for (int s = 0; s < P.n_strains; ++s)
                                if (!P.immunity || x[s] >= 0 || R.Bernoulli(1 + x[s])) // If there is no immunity...
                                    if (R.Bernoulli(P.theta[s])) // and transfer is successful ...
                                        { x[s] = max(x[s], 0.0) + P.iota * xx[s]; normalize = true; }
                            if (normalize)
                                Normalize(x);
                        }
                        break;
                }
            }

            // 4. Report per-strain carriage, average multiplicity of carriage, and distribution of multiplicity of carriage to screen and output file
            if (g % P.report == 0)
            {
                if (filename != P.fileout)  // If needed, open new file and print header
                {
                    filename = P.fileout;
                    if (fout.is_open())
                        fout.close();
                    fout.open(P.fileout);

                    sout << "run\ttau\tt";
                    for (int e = 0; e < P.n_strains / 2; ++e)
                        sout << "\t" << string(1 + e / 26, char('A' + e % 26)) << "s\t" << string(1 + e / 26, char('A' + e % 26)) << "r";
                    sout << "\tmult\tcarr0\tcarr1\tcarr2\tcarr3\tcarr4\tcarr5\tcarr6\tcarr7\tcarr8plus\n";
                }

                sout << run << "\t" << P.tau << "\t" << g * P.t_step;
                for (auto ll : l)
                    sout << "\t" << ll;

                vector<int> strain_count(9, 0), serotype_count(9, 0);
                double mult = 0, STmult = 0, carriers = 0;
                for (int i = 0; i < P.n_strains * P.n_hosts; i += P.n_strains)
                {
                    int m = count_if(X.begin() + i, X.begin() + i + (P.first_sero ? 2 : P.n_strains), [](double x) { return x > 0; });
                    if (m > 0) { ++carriers; mult += m; }
                    ++strain_count[min(8, m)];
                }
                sout << "\t" << mult / carriers;
                for (auto s : strain_count)
                    sout << "\t" << s;

                cout << sout.str() << "\n";
                fout << sout.str() << "\n";
                sout.str(string());
            }
        }
    }

    return 0;
}