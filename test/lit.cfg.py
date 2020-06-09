import lit.formats
from lit.llvm import llvm_config

config.name = 'Nacro'
config.test_format = lit.formats.ShTest(True)

config.suffixes = ['.c', '.cpp', '.cc']

config.excludes = ['CMakeLists.txt']

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.nacro_obj_root, 'test')

config.substitutions.append(('%clang',
    os.path.join(config.llvm_bin_dir, 'clang')))
config.substitutions.append(('%FileCheck', config.filecheck_path))
# FIXME: What about .dylib?
config.substitutions.append(('%NacroPlugin',
    os.path.join(config.nacro_obj_root, 'NacroPlugin.so')))
