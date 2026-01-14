require 'json'

package = JSON.parse(File.read(File.join(__dir__, 'package.json')))

Pod::Spec.new do |s|
  s.name         = "arfit-kit-react-native"
  s.version      = package['version']
  s.summary      = package['description']
  s.homepage     = package['repository']['url']
  s.license      = package['license']
  s.authors      = package['author']

  s.platforms    = { :ios => "14.0" }
  s.source       = { :git => package['repository']['url'], :tag => "v#{s.version}" }

  s.source_files = "ios/**/*.{h,m,mm}"
  
  s.dependency "React-Core"
  
  # Link to ARFitKit core (if built as a framework)
  # s.dependency "ARFitKit"
end
