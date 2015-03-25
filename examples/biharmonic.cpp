/** @file biharmonic.cpp

    @brief A Biharmonic example.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): J. Sogn
*/

# include <gismo.h>
# include <gsAssembler/gsBiharmonicAssembler.h>
# include <gsAssembler/gsSeminormH2.h>
using std::cout;
using std::endl;
using namespace gismo;


int main(int argc, char *argv[])
{

    index_t numRefine = 5;
    index_t numDegree = 1;
    dirichlet::strategy dirStrategy = dirichlet::elimination;
    iFace::strategy intStrategy = iFace::glue;

    gsFunctionExpr<> solution("(cos(4*pi*x) - 1) * (cos(4*pi*y) - 1)");
    gsFunctionExpr<> source  ("256*pi*pi*pi*pi*(4*cos(4*pi*x)*cos(4*pi*y) - cos(4*pi*x) - cos(4*pi*y))");
    gsFunctionExpr<> laplace ("-16*pi*pi*(2*cos(4*pi*x)*cos(4*pi*y) - cos(4*pi*x) - cos(4*pi*y))");
    gsMFunctionExpr<>sol1der ("-4*pi*(cos(4*pi*y) - 1)*sin(4*pi*x)",
                              "-4*pi*(cos(4*pi*x) - 1)*sin(4*pi*y)");
    gsMFunctionExpr<>sol2der ("-16*pi^2*(cos(4*pi*y) - 1)*cos(4*pi*x)",
                              "-16*pi^2*(cos(4*pi*x) - 1)*cos(4*pi*y)",
                              " 16*pi^2*sin(4*pi*x)*sin(4*pi*y)", 2);

    gsMultiPatch<> * geo;
    geo = new gsMultiPatch<>( *safe(gsNurbsCreator<>::BSplineFatQuarterAnnulus()) );
    gsMultiBasis<> basis = gsMultiBasis<>(*geo);
    //p-refine to get equal polynomial degree s,t directions (for Annulus)
    basis.degreeElevateComponent(0);

    for (int i = 0; i < numDegree; ++i)
        basis.degreeElevate();
    for (int i = 0; i < numRefine; ++i)
        basis.uniformRefine();


    //Setting up oundary conditions
    gsBoundaryConditions<> bcInfo;
    bcInfo.addCondition( boundary::west,  condition_type::dirichlet, &solution);//Annulus: small arch lenght
    bcInfo.addCondition( boundary::east,  condition_type::dirichlet, &solution);//Annulus: Large arch lenght
    bcInfo.addCondition( boundary::north, condition_type::dirichlet, &solution);
    bcInfo.addCondition( boundary::south, condition_type::dirichlet, &solution);
    //Neumann condition of second kind
    gsBoundaryConditions<> bcInfo2;
    bcInfo2.addCondition( boundary::west,  condition_type::neumann, &laplace);
    bcInfo2.addCondition( boundary::east,  condition_type::neumann, &laplace);
    bcInfo2.addCondition( boundary::north, condition_type::neumann, &laplace);
    bcInfo2.addCondition( boundary::south, condition_type::neumann, &laplace);

    //Initilize solver
    gsBiharmonicAssembler<real_t> BiharmonicAssembler(*geo,basis,bcInfo,bcInfo2,source,
                                                       dirStrategy, intStrategy);

    std::cout<<"Assembling..." << std::endl;
    BiharmonicAssembler.assemble();

    cout<<"Solving with direct solver, "<< BiharmonicAssembler.numDofs()<< " DoFs..."<< endl;
    Eigen::SparseQR< gsSparseMatrix<>, Eigen::COLAMDOrdering<int> > solver;
    solver.analyzePattern(BiharmonicAssembler.matrix() );
    solver.factorize(BiharmonicAssembler.matrix());
    gsMatrix<> solVector= solver.solve(BiharmonicAssembler.rhs());

    //Reconstruct solution
    gsField<>::uPtr solField = safe(BiharmonicAssembler.constructSolution(solVector));

    //Contruct the H2 norm, part by part.
    gsSeminormH2<real_t> h2Seminorm = gsSeminormH2<real_t>(*solField,solution,sol2der);
    h2Seminorm.compute();
    real_t errorH2Semi = h2Seminorm.value();

    gsSeminormH1<real_t> h1Seminorm = gsSeminormH1<real_t>(*solField,solution,sol1der);
    h1Seminorm.compute();
    real_t errorH1Semi = h1Seminorm.value();

    gsNormL2<real_t> L2Norm = gsNormL2<real_t>(*solField,solution);
    L2Norm.compute();
    real_t errorL2 = L2Norm.value();

    real_t errorH2 = math::sqrt(errorH2Semi*errorH2Semi + errorH1Semi*errorH1Semi + errorL2*errorL2);

    cout << "The H2 error of the solution is : " << errorH2 << endl;
    cout << "The L2 error of the solution is : " << errorL2 << endl;


    // Plot solution in paraview
    bool plot = false;
    if (plot)
    {
        // Write approximate and exact solution to paraview files
        std::cout<<"Plotting in Paraview...\n";
        gsWriteParaview<>(*solField, "Biharmonic2d", 5000);
        const gsField<> exact( *geo, solution, false );
        gsWriteParaview<>( exact, "Biharmonic2d_exact", 5000);
    }

    delete geo;

    return  0;
}