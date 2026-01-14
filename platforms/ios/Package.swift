// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "ARFitKit",
    platforms: [
        .iOS(.v14),
        .macOS(.v12)
    ],
    products: [
        .library(
            name: "ARFitKit",
            targets: ["ARFitKit"]
        ),
    ],
    dependencies: [],
    targets: [
        .target(
            name: "ARFitKit",
            dependencies: ["ARFitKitCore"],
            path: "Sources/ARFitKit"
        ),
        .target(
            name: "ARFitKitCore",
            dependencies: [],
            path: "Sources/ARFitKitCore",
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath("../../core/include")
            ]
        ),
        .testTarget(
            name: "ARFitKitTests",
            dependencies: ["ARFitKit"],
            path: "Tests/ARFitKitTests"
        ),
    ],
    cxxLanguageStandard: .cxx17
)
