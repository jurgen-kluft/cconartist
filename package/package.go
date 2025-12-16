package cconartist

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	"github.com/jurgen-kluft/ccode/denv"
	cgui "github.com/jurgen-kluft/cgui/package"
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
	cbasepkg := cbase.GetPackage()
	clibuvpkg := clibuv.GetPackage()
	cmmiopkg := cmmio.GetPackage()
	cguiapppkg := cgui.GetPackage()
	cjsonpkg := cjson.GetPackage()
	cunittestpkg := cunittest.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(cbasepkg)
	mainpkg.AddPackage(clibuvpkg)
	mainpkg.AddPackage(cmmiopkg)
	mainpkg.AddPackage(cguiapppkg)
	mainpkg.AddPackage(cjsonpkg)
	mainpkg.AddPackage(cunittestpkg)

	// main library
	mainlib := denv.SetupCppLibProject(mainpkg, name)
	mainlib.AddDependencies(cbasepkg.GetMainLib())
	mainlib.AddDependencies(clibuvpkg.GetMainLib())
	mainlib.AddDependencies(cmmiopkg.GetMainLib())

	// server application
	cconartist_server := denv.SetupCppAppProjectForDesktop(mainpkg, "cconartist_server", "server")
	cconartist_server.CopyToOutput("source/main/plugins", "*.dylib", "plugins")
	cconartist_server.CopyToOutput("config", "*.json", "")
	cconartist_server.AddDependency(mainlib)
	cconartist_server.AddDependencies(cjsonpkg.GetMainLib())
	cconartist_server.AddDependencies(cguiapppkg.GetMainLib())

	// gui application
	cconartist_gui := denv.SetupCppAppProjectForDesktop(mainpkg, "cconartist_gui", "gui")
	cconartist_gui.CopyToOutput("source/main/plugins", "*.dylib", "plugins")
	cconartist_gui.AddDependency(mainlib)
	cconartist_gui.AddDependencies(cjsonpkg.GetMainLib())
	cconartist_gui.AddDependencies(cguiapppkg.GetMainLib())

	// test library
	testlib := denv.SetupCppTestLibProject(mainpkg, name)
	testlib.AddDependencies(cbasepkg.GetTestLib())
	testlib.AddDependencies(clibuvpkg.GetTestLib())
	testlib.AddDependencies(cmmiopkg.GetTestLib())
	testlib.AddDependencies(cunittestpkg.GetTestLib())

	// unittest project
	maintest := denv.SetupCppTestProject(mainpkg, name)
	maintest.AddDependencies(cunittestpkg.GetMainLib())
	maintest.AddDependency(testlib)

	mainpkg.AddMainApp(cconartist_server)
	mainpkg.AddMainApp(cconartist_gui)
	mainpkg.AddMainLib(mainlib)
	mainpkg.AddTestLib(testlib)
	mainpkg.AddUnittest(maintest)

	return mainpkg
}
