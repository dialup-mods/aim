#include "doctest/doctest.h"
#include "AdapterRegistry.h"
#include "PluginAdapter.h"
#include "TestModules.h"
#include "MockLibraryLoader.h"

struct AdapterRegistryFixture {
    AdapterRegistry& registry = AdapterRegistry::getInstance();

    AdapterRegistryFixture() {
        registry.clear();
    }

    ~AdapterRegistryFixture() {
        registry.clear();
    }
};

TEST_CASE("AdapterRegistry registers and resolves modules") {
    // Create a test adapter (mimicking DialUp-Core setup)
    auto adapter = AdapterBuilder()
        .named("TestAdapter")
        .asFramework()
        .build();

    adapter->getContainer()->registerInstance<TestDep>(std::make_shared<TestDep>());
    
    AdapterRegistry::getInstance().add("TestAdapter", std::move(adapter));
    
    // Verify retrieval
    auto* retrieved = AdapterRegistry::getInstance().get("TestAdapter");
    REQUIRE(retrieved != nullptr);
    
    auto dep = retrieved->getContainer()->resolve<TestDep>();
    CHECK(dep.get() != nullptr);
    CHECK(dep->getValue() == "test_value");
    AdapterRegistry::getInstance().clear();
}

struct ResolverFixture {
    std::unique_ptr<Resolver> pluginResolver;
    // Setup framework adapter

    ResolverFixture() {
        AdapterRegistry::getInstance().add("DialUp-Core",
            AdapterBuilder()
            .named("DialUp-Core")
            .asFramework()
            .build());
        AdapterRegistry::getInstance().get("DialUp-Core")->setPluginReady();

        AdapterRegistry::getInstance().add("PublicInterfaces",
            AdapterBuilder()
            .named("PublicInterfaces")
            .asPublicInterfaces()
            .build());
        AdapterRegistry::getInstance().get("PublicInterfaces")->setPluginReady();

        AdapterRegistry::getInstance().add("AIM",
            AdapterBuilder()
            .named("AIM")
            .asCorePlugin()
            .build());
        //AdapterRegistry::getInstance().get("AIM")->setPluginReady();

        AdapterRegistry::getInstance().add("TestPlugin",
            AdapterBuilder()
            .named("TestPlugin")
            .build());
        //AdapterRegistry::getInstance().get("TestPlugin")->setPluginReady();

        //AdapterRegistry::getInstance().add("TestPlugin", std::move(pluginAdapter));
        //auto resolvedPlugin = AdapterRegistry::getInstance().get("TestPlugin");
        //test->instantiateIPlugin();

        //printf("adapters:\n");
        //for (auto adapter : registry.getAll()) {
        //    printf("  name: %s\n", adapter->getName().c_str());
        //    printf("  ptr: %p\n", adapter);
        //}

        //// Create resolver for the plugin
        //pluginResolver = std::make_unique<Resolver>(
        //    resolvedPlugin->getContainer(),
        //    false  // resolveSelfOnly
        //);
        //printf("pluginResolver ptr: %p\n", pluginResolver.get());
    }

    ~ResolverFixture() {
        printf("clearing\n");
        AdapterRegistry::getInstance().clear();
    }
};

TEST_CASE_FIXTURE(ResolverFixture, "resolves dep from self when exists in self, framework, and public interfaces") {
    MESSAGE("NEW TEST\n\n\n");
    auto dep = std::make_shared<TestDep>();
    auto coreDep = std::make_shared<TestDep>();
    auto publicDep = std::make_shared<TestDep>();

    auto resolver = AdapterRegistry::getInstance().get("TestPlugin")->getResolver();
    printf("pluginResolver from registry ptr: %p\n", resolver);

    AdapterRegistry::getInstance().get("TestPlugin")->getContainer()->registerInstance<TestDep>(dep);
    AdapterRegistry::getInstance().get("DialUp-Core")->getContainer()->registerInstance<TestDep>(coreDep);
    AdapterRegistry::getInstance().get("PublicInterfaces")->getContainer()->registerInstance<TestDep>(publicDep);

    auto resolved = resolver->resolve<TestDep>();

    CHECK(resolved.get() == dep.get());
}

TEST_CASE_FIXTURE(ResolverFixture, "plugin resolves from self when dep only exists in self") {
    MESSAGE("NEW TEST\n\n\n");
    auto dep = std::make_shared<TestDep>();
    AdapterRegistry::getInstance().get("TestPlugin")->getContainer()->registerInstance<TestDep>(dep);

    auto resolved = AdapterRegistry::getInstance().get("TestPlugin")->getResolver()->resolve<TestDep>();
    CHECK(resolved.get() == dep.get());
}

TEST_CASE_FIXTURE(ResolverFixture, "resolves framework dep when no self or public interface dep exists") {
    MESSAGE("NEW TEST\n\n\n");
    auto dep = std::make_shared<TestDep>();
    // not registered on self/pluginAdapter
    // not registered on dialup-core
    printf("adapters:\n");
    for (auto adapter : AdapterRegistry::getInstance().getAll()) {
        printf("  name: %s\n", adapter->getName().c_str());
        printf("  ptr: %p\n", adapter);
    }

    auto resolver = AdapterRegistry::getInstance().get("TestPlugin")->getResolver();
    AdapterRegistry::getInstance().get("DialUp-Core")->getContainer()->registerInstance<TestDep>(dep);

    auto resolved = resolver->resolve<TestDep>();
    CHECK(resolved.get() == dep.get());
}

TEST_CASE_FIXTURE(ResolverFixture, "resolves public interface dep when no self or framework dep exists") {
    MESSAGE("NEW TEST\n\n\n");
    auto publicDep = std::make_shared<TestDep>();
    // not registered on self/pluginAdapter
    // not registered on dialup-core
    AdapterRegistry::getInstance().get("PublicInterfaces")->getContainer()->registerInstance<TestDep>(publicDep);

    auto resolved = AdapterRegistry::getInstance().get("TestPlugin")->getResolver()->resolve<TestDep>();
    CHECK(resolved.get() == publicDep.get());
}

TEST_CASE_FIXTURE(ResolverFixture, "self takes precedence over core") {
    MESSAGE("NEW TEST\n\n\n");
    auto selfDep = std::make_shared<TestDep>();
    auto frameworkDep = std::make_shared<TestDep>();

    // not registered in public interfaces
    AdapterRegistry::getInstance().get("TestPlugin")->getContainer()->registerInstance<TestDep>(selfDep);
    AdapterRegistry::getInstance().get("DialUp-Core")->getContainer()->registerInstance<TestDep>(frameworkDep);

    auto resolved = AdapterRegistry::getInstance().get("TestPlugin")->getResolver()->resolve<TestDep>();
    CHECK(resolved.get() == selfDep.get());
}

TEST_CASE_FIXTURE(ResolverFixture, "self takes precedence over public interface") {
    MESSAGE("NEW TEST\n\n\n");
    auto selfDep = std::make_shared<TestDep>();
    auto interfaceDep = std::make_shared<TestDep>();

    AdapterRegistry::getInstance().get("TestPlugin")->getContainer()->registerInstance<TestDep>(selfDep);
    AdapterRegistry::getInstance().get("PublicInterfaces")->getContainer()->registerInstance<TestDep>(interfaceDep);

    auto resolved = AdapterRegistry::getInstance().get("TestPlugin")->getResolver()->resolve<TestDep>();
    CHECK(resolved.get() == selfDep.get());
}