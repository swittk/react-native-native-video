import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';

/**
 * The native module is intentionally tiny. In a New Architecture host it uses
 * the supported JSI bindings installer; in a legacy host, resolving the bridge
 * module initializes its runtime installer. Video and frame values remain C++
 * HostObjects because Codegen cannot model those identities or their direct
 * ArrayBuffer access.
 */
export interface Spec extends TurboModule {
  readonly multiply: (a: number, b: number) => number;
}

export default TurboModuleRegistry.getEnforcing<Spec>('NativeVideo');
