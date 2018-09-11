// config_def.h
// Contains definitions of all config parameters to be loaded from the
// configuration file or the command line.

PARAMETER ( int,            n_strains,      2 );            // number of strains. indexed from 0, even-numbered strains are sensitive, odd-numbered strains are resistant
PARAMETER ( int,            n_hosts,        10000 );        // number of hosts
PARAMETER ( vector<double>, w,              { 1, 1 } );     // within-host fitness (growth rate per unit time) of each strain
PARAMETER ( vector<double>, beta,           { 4, 4 } );     // transmission rate of each strain
PARAMETER ( double,         gamma,          0.0 );          // contact rate for transfer (whole-carriage transmission)
PARAMETER ( vector<double>, theta,          { 1, 1 } );     // success probability of transfer for each strain (independent of beta, gamma)
PARAMETER ( vector<double>, u,              { 1 } );        // natural clearance rate of each serotype (each adjacent pair of sensitive/resistant strains are the same serotype)
PARAMETER ( double,         v,              0.0 );          // probability of clearing all carried serotypes when any serotype gets cleared
PARAMETER ( double,         k,              1.0 );          // relative efficiency of co-colonisation (must be 0 <= k <= 1)
PARAMETER ( double,         tau,            0.1 );          // antibiotic treatment rate
PARAMETER ( double,         iota,           1e-3 );         // germ size
PARAMETER ( double,         phi,            0.0 );          // strength of within-host negative frequency-dependent selection
PARAMETER ( double,         min_carriage,   3e-5 );         // when carriage of a given strain goes below this proportion, it gets eliminated through within-host competition (non-immunising)
PARAMETER ( double,         min_carriers,   1 );            // to avoid complete loss of strains, this is the minimum number of carriers counted for each strain
PARAMETER ( double,         init,           0.1 );          // initial fraction of the population who are infected
PARAMETER ( bool,           immunity,       false );        // whether natural clearance is immunising (immunity is assumed to be for life)
PARAMETER ( double,         sigma,          1.0 );          // degree of immune protection following clearance
PARAMETER ( double,         birth_rate,     0 );            // rate at which new, immunologically-naive and uncolonised individuals are introduced into the population
PARAMETER ( double,         t_max,          24 );           // how long to run the simulation for
PARAMETER ( double,         t_step,         0.001 );        // time step granularity
PARAMETER ( string,         fileout,        "./out.txt" );  // output file
PARAMETER ( int,            report,         1000 );         // how often to save steps
PARAMETER ( bool,           first_sero,     false );        // if true, only count first serotype when tallying number of carriers with 0, 1, 2, etc. strains