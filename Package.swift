// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "ARFitKit",
    platforms: [
        .iOS(.v13)
    ],
    products: [
        // The external product interface
        .library(
            name: "ARFitKit",
            targets: ["ARFitKit"]),
    ],
    dependencies: [
        // Specify dependencies here
        // .package(url: "https://github.com/google/mediapipe.git", from: "0.10.0"), // If using remote MP
    ],
    targets: [
        // Core C++ Target
        .target(
            name: "ARFitKitCore",
            dependencies: [],
            path: "core",
            exclude: [
                "CMakeLists.txt", 
                "tests", 
                "examples",
                "src/gpu/vulkan_backend.cpp", // Exclude non-iOS files
                "src/gpu/webgpu_backend.cpp"
            ],
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath("include"),
                .define("ARFIT_USE_METAL", .when(platforms: [.iOS])),
                .define("ARFIT_USE_GPU", .when(platforms: [.iOS])),
                .unsafeFlags(["-std=c++17"])
            ]
        ),
        // Swift / Obj-C++ Bridge Target
        .target(
            name: "ARFitKit",
            dependencies: ["ARFitKitCore"],
            path: "platforms/ios/Sources",
            sources: ["ARFitKit", "ARFitKitCore"], // Include Swift and Obj-C++ bridge sources
            publicHeadersPath: "ARFitKitCore/include",
            cxxSettings: [
                .headerSearchPath("../../core/include")
            ]
        )
    ],
    cxxLanguageStandard: .cxx17
)
