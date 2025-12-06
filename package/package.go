package cconartist

import (
	"github.com/jurgen-kluft/ccode/denv"
	ccore "github.com/jurgen-kluft/ccore/package"
	cguiapp "github.com/jurgen-kluft/cguiapp/package"
	cjson "github.com/jurgen-kluft/cjson/package"
	clibuv "github.com/jurgen-kluft/clibuv/package"
	cmmio "github.com/jurgen-kluft/cmmio/package"
	cunittest "github.com/jurgen-kluft/cunittest/package"
)

const (
	repo_path = "github.com\\jurgen-kluft"
	repo_name = "cconartist"
)

func GetPackage() *denv.Package {
	name := repo_name

	// dependencies
	ccorepkg := ccore.GetPackage()
	clibuvpkg := clibuv.GetPackage()
	cmmiopkg := cmmio.GetPackage()
	cguiapppkg := cguiapp.GetPackage()
	cjsonpkg := cjson.GetPackage()
	cunittestpkg := cunittest.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(ccorepkg)
	mainpkg.AddPackage(clibuvpkg)
	mainpkg.AddPackage(cmmiopkg)
	mainpkg.AddPackage(cguiapppkg)
	mainpkg.AddPackage(cjsonpkg)
	mainpkg.AddPackage(cunittestpkg)

	// main library
	mainlib := denv.SetupCppLibProject(mainpkg, name)
	mainlib.AddDependencies(ccorepkg.GetMainLib())
	mainlib.AddDependencies(clibuvpkg.GetMainLib())
	mainlib.AddDependencies(cmmiopkg.GetMainLib())

	// test library
	testlib := denv.SetupCppTestLibProject(mainpkg, name)
	testlib.AddDependencies(ccorepkg.GetTestLib())
	testlib.AddDependencies(clibuvpkg.GetTestLib())
	testlib.AddDependencies(cmmiopkg.GetTestLib())
	testlib.AddDependencies(cunittestpkg.GetTestLib())

	// main application
	cconartist := denv.SetupCppAppProjectForDesktop(mainpkg, "cconartist", "capp")
	cconartist.CopyToOutput("source/main/plugins", "*.dylib", "plugins")
	cconartist.AddDependency(mainlib)
	cconartist.AddDependencies(cjsonpkg.GetMainLib())
	cconartist.AddDependencies(cguiapppkg.GetMainLib())

	// unittest project
	maintest := denv.SetupCppTestProject(mainpkg, name)
	maintest.AddDependencies(cunittestpkg.GetMainLib())
	maintest.AddDependency(testlib)

	mainpkg.AddMainApp(cconartist)
	mainpkg.AddMainLib(mainlib)
	mainpkg.AddTestLib(testlib)
	mainpkg.AddUnittest(maintest)

	return mainpkg
}
