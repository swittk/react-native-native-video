require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))
cxx_language_standard = defined?(rct_cxx_language_standard) ? rct_cxx_language_standard() : "c++17"

Pod::Spec.new do |s|
  s.name         = "react-native-native-video"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  # Preserve the package's original floor for the Monterey-era RN 0.64 example.
  # Modern RN/Expo applications impose their own higher application floor.
  s.platforms    = { :ios => "10.0" }
  s.source       = {
    :git => "https://github.com/swittk/react-native-native-video.git",
    :tag => "#{s.version}"
  }

  s.source_files = "ios/**/*.{h,m,mm}", "cpp/**/*.{h,cpp}"
  s.frameworks = "AVFoundation", "Accelerate", "CoreImage", "UIKit"
  s.requires_arc = true
  s.pod_target_xcconfig = {
    "CLANG_CXX_LANGUAGE_STANDARD" => cxx_language_standard
  }

  if defined?(install_modules_dependencies)
    # RN 0.71+ supplies codegen and TurboModule dependencies through this helper.
    install_modules_dependencies(s)
  else
    # RN 0.64 has no install_modules_dependencies helper.
    s.dependency "React-Core"
    s.dependency "React-callinvoker"
    s.dependency "React-jsi"
  end
end
