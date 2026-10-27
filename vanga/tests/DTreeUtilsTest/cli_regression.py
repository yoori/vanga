import csv
import io
import json
import copy
import math
import pathlib
import subprocess
import sys
import tempfile


def main():
    binaries = pathlib.Path(sys.argv[1]).resolve()

    def run(program, *arguments, **kwargs):
        expected_success = kwargs.pop('success', True)
        result = subprocess.run(
            [str(binaries / program)] + [str(arg) for arg in arguments],
            universal_newlines=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=30, **kwargs)
        if (result.returncode == 0) != expected_success:
            raise AssertionError('{} {}: exit {}\n{}\n{}'.format(
                program, arguments, result.returncode,
                result.stdout, result.stderr))
        return result

    with tempfile.TemporaryDirectory(prefix='vanga-regression-') as directory:
        root = pathlib.Path(directory)
        data = root / 'train.libsvm'
        data.write_text(('1 42:1\n0 42:0\n') * 10)

        missing = run('SVMUtil', 'correlate', root / 'missing', success=False)
        assert "can't open source file" in missing.stderr
        filtered = run('SVMUtil', 'correlate', data)
        assert len(filtered.stdout.splitlines()) == 20

        tree = root / 'predict.model'
        tree.write_text('dtree\n1\t0\t42:2:3\n2\t2\t\n3\t-2\t\n')
        test_data = root / 'predict.libsvm'
        test_data.write_text('0 42:0\n0\n1 42:1\n')
        predictions = list(map(float, run(
            'DTreeTrainer', 'predict', tree, test_data).stdout.splitlines()))
        assert len(predictions) == 3
        assert abs(predictions[0] - 1.0 / (1.0 + math.exp(2))) < 1e-7
        assert predictions[0] == predictions[1]
        assert predictions[2] > 0.8

        numeric_json = root / 'catboost.json'
        numeric_model = {
            'features_info': {'float_features': [
                {'feature_index': 0, 'flat_feature_index': 0,
                 'nan_value_treatment': 'AsTrue'}]},
            'scale_and_bias': [2.0, [0.25]],
            'oblivious_trees': [{'splits': [
                {'split_type': 'FloatFeature', 'float_feature_index': 0, 'border': -0.5}],
                'leaf_values': [-1.25, 2.5]}],
        }
        numeric_json.write_text(json.dumps(numeric_model))
        numeric_native = root / 'numeric.vanga'
        run('DTreeTrainer', 'import-catboost', numeric_json, numeric_native)
        assert numeric_native.read_text().startswith('dtree-float\n')
        numeric_data = root / 'numeric.libsvm'
        numeric_data.write_text('0 1:-2\n0 1:-0.5\n0\n0 1:nan\n0 1:0.5\n')
        for imported_model in (numeric_json, numeric_native):
            raw = list(map(float, run('DTreeTrainer', 'predict-raw',
                                     imported_model, numeric_data).stdout.splitlines()))
            assert raw == [-2.25, -2.25, 5.25, 5.25, 5.25]

        mutations = [
            lambda m: m.update({'trees': []}),
            lambda m: m.update({'scale_and_bias': [1, [0, 0]]}),
            lambda m: m['features_info'].update({'categorical_features': [{}]}),
            lambda m: m['oblivious_trees'][0].update({'leaf_values': [0]}),
            lambda m: m['oblivious_trees'][0]['splits'][0].update({'split_type': 'OnlineCtr'}),
            lambda m: m['oblivious_trees'][0]['splits'][0].update({'float_feature_index': 3}),
        ]
        for mutate in mutations:
            invalid_json = copy.deepcopy(numeric_model)
            mutate(invalid_json)
            numeric_json.write_text(json.dumps(invalid_json))
            rejected = run('DTreeTrainer', 'import-catboost', numeric_json,
                           root / 'invalid.vanga', success=False)
            assert 'CatBoost JSON:' in rejected.stderr
        for invalid_row in ('0 0:1\n', '0 1:2 1:3\n', '0 1:broken\n'):
            numeric_data.write_text(invalid_row)
            run('DTreeTrainer', 'predict', numeric_native, numeric_data, success=False)

        options = ['--steps=1', '--train-bags=0',
                   '--metric-sqd=0', '--metric-logloss=1']
        unsupported_resume = run('DTreeTrainer', 'train-add', numeric_native,
                                 data, *options, success=False)
        assert 'do not support numeric models' in unsupported_resume.stderr
        model = root / 'train.model'
        # Fresh training must ignore the contents of an existing output file.
        model.write_text('obsolete model')
        run('DTreeTrainer', 'train', model, data, *options)
        assert model.read_text().startswith('dtree\n')
        run('DTreeTrainer', 'predict', model, data)
        continued = run('DTreeTrainer', 'train-add', model, data, *options)
        assert 'loaded model:' in continued.stdout
        run('DTreeTrainer', 'predict', model, data)
        run('DTreeTrainer', 'train-add', root / 'missing.model', data,
            *options, success=False)
        legacy = root / 'legacy.model'
        run('DTreeTrainer', 'train-trees', legacy, data, *options)
        resumed = run('DTreeTrainer', 'train-trees', legacy, data, *options)
        assert 'loaded model:' in resumed.stdout

        storage = root / 'profiles'
        run('SegmentUtil', 'profile', 'id,timestamp,hashes',
            '--storage=' + str(storage), input='u1,100,42\nu1,110,42\n')
        profile = run('SegmentUtil', 'print', '--user-id=u1',
                      '--storage=' + str(storage))
        assert '42' in profile.stdout
        request = run('SegmentUtil', 'request', '+id,timestamp',
                      '--storage=' + str(storage), '--segment-config=30:1',
                      input='u1,120\n')
        requested = list(csv.reader(io.StringIO(request.stdout)))
        assert len(requested) == 1
        assert requested[0][0] == 'u1' and requested[0][1]


if __name__ == '__main__':
    main()
