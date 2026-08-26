/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const REDACTED: string = '[redacted]';

const SENSITIVE_KEYS: Set<string> = new Set<string>([
  'tokenId',
  'tokenID',
  'deviceId',
  'deviceID',
  'deviceName',
  'portId',
  'portName',
  'accessory',
  'appName',
  'productName',
  'serial',
  'serialNumber',
  'sn',
  'iSerialNumber',
  'manufacturer',
  'manufacturerName',
  'model',
  'modelName',
  'vendor',
  'vendorName'
]);

interface MaskedWant {
  abilityName?: string;
  bundleName?: string;
  action?: string;
  parameters?: Object;
}

interface MaskedError {
  name: string;
  code?: Object;
}

function isSensitiveKey(key: string): boolean {
  return SENSITIVE_KEYS.has(key);
}

function deepMask(obj: Object): Object {
  if (obj instanceof Array) {
    const result: Object[] = [];
    for (let i = 0; i < obj.length; i++) {
      result.push(deepMask(obj[i] as Object));
    }
    return result;
  }
  const record = obj as Record<string, Object>;
  const result: Record<string, Object> = {};
  const keys: string[] = Object.keys(record);
  for (let i = 0; i < keys.length; i++) {
    const key: string = keys[i];
    if (isSensitiveKey(key)) {
      continue;
    }
    result[key] = deepMask(record[key] as Object);
  }
  return result;
}

export function maskWant(want: Object): string {
  try {
    const w = want as MaskedWant;
    const sanitized: MaskedWant = {};
    if (w.abilityName !== undefined) {
      sanitized.abilityName = w.abilityName;
    }
    if (w.bundleName !== undefined) {
      sanitized.bundleName = w.bundleName;
    }
    if (w.action !== undefined) {
      sanitized.action = w.action;
    }
    const parameters = w.parameters;
    if (parameters !== undefined) {
      sanitized.parameters = deepMask(parameters as Object);
    }
    return JSON.stringify(sanitized);
  } catch (e) {
    return REDACTED;
  }
}

export function maskObject(obj: Object): string {
  try {
    if (obj instanceof Error) {
      const err = obj as Object as MaskedError;
      const result: MaskedError = { name: err.name };
      const code = err.code;
      if (code !== undefined) {
        result.code = code;
      }
      return JSON.stringify(result);
    }
    return JSON.stringify(deepMask(obj));
  } catch (e) {
    return REDACTED;
  }
}
