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

const MASK: string = '***';

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
      result[key] = MASK;
    } else {
      result[key] = deepMask(record[key] as Object);
    }
  }
  return result;
}

export function maskWant(want: Object): string {
  try {
    const w = want as Record<string, Object>;
    const sanitized: Record<string, Object> = {};
    const abilityName = w['abilityName'];
    if (abilityName !== undefined) {
      sanitized['abilityName'] = abilityName;
    }
    const bundleName = w['bundleName'];
    if (bundleName !== undefined) {
      sanitized['bundleName'] = bundleName;
    }
    const action = w['action'];
    if (action !== undefined) {
      sanitized['action'] = action;
    }
    const parameters = w['parameters'];
    if (parameters !== undefined) {
      sanitized['parameters'] = deepMask(parameters);
    }
    return JSON.stringify(sanitized);
  } catch (e) {
    return MASK;
  }
}

export function maskObject(obj: Object): string {
  try {
    if (obj instanceof Error) {
      const err: Record<string, Object> = { 'name': obj.name, 'message': MASK };
      const errRecord = obj as Record<string, Object>;
      const code = errRecord['code'];
      if (code !== undefined) {
        err['code'] = code;
      }
      return JSON.stringify(err);
    }
    return JSON.stringify(deepMask(obj));
  } catch (e) {
    return MASK;
  }
}
