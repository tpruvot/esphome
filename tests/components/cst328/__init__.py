import esphome.codegen as cg
from esphome.types import ConfigType
from tests.testing_helpers import ComponentManifestOverride


def override_manifest(manifest: ComponentManifestOverride) -> None:
    async def to_code_testing(config: ConfigType) -> None:
        cg.add_define("USE_CST328_CST3240")

    manifest.to_code = to_code_testing
    # touchscreen.h includes display.h
    manifest.dependencies = manifest.dependencies + ["display"]
